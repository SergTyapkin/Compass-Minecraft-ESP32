#include <Arduino.h>
#include <FileData.h>
#include <GTimer.h>
#include <LittleFS.h>
#include <QMC5883L.h>
#include <TinyGPSPlus.h>
#include <Wire.h>
#include <uButton.h>

#include "config.h"
#include "icons.h"
#include "matrix.h"
#include "battery.h"

/*
    удержание - яркость
    1x клик:
        в режиме GPS - показать расстояние до точки, кликнуть ещё раз - направление
    2x клик - смена режима:
        круг - компас
        треугольник - GPS
    3x клик:
        в режиме компаса - калибровка компаса
        в режиме GPS - запомнить точку
*/

enum class Mode {
    Charging,
    Compass,
    TargetDir,
    TargetDist,
};

struct Data {
    uint8_t brightness = 30;
    Mode mode = Mode::Compass;
    MagCal cal;
    float lat;
    float lng;
};
Data cfg;
FileData fdata(&LittleFS, "/cfg.cfg", 'A', &cfg, sizeof(cfg));

uButton btn(BTN_PIN);

QMC5883L mag(I2C_MAG_ADDR);

TinyGPSPlus gps;

Battery battery(BATTERY_CHARGING_PIN, BATTERY_FULL_PIN);

static uint32_t compassColors[] = {
    Adafruit_NeoPixel::Color(255, 0, 0),
    Adafruit_NeoPixel::Color(60, 0, 0),
    Adafruit_NeoPixel::Color(120, 120, 120),
};

static uint32_t GPSColors[] = {
    Adafruit_NeoPixel::Color(0, 255, 0),
    Adafruit_NeoPixel::Color(0, 60, 0),
    Adafruit_NeoPixel::Color(120, 120, 120),
};

// ===== BUZZER SETTINGS =====
#define BUZZER_PWM_CHANNEL 1
#define BUZZER_PWM_RESOLUTION 8
#define BUZZER_TONE_DURATION 150  // ms per note

// ===== BUZZER STATE =====
static bool victoryPointReached = false;
static unsigned long victoryStartTime = 0;
static int victoryNoteIndex = 0;
static bool victoryPlaying = false;

#define PROGRESS_BAR_HEGIHT 4
#define PROGRESS_BAR_WIDTH LED_MATRIX_WIDTH
#define PROGRESS_BAR_Y (LED_MATRIX_HEIGHT - PROGRESS_BAR_HEGIHT) / 2
bool onCalibrate(const MagCalProgress& p) {
    disp.clear();
    disp.color = 0xffff00; // yellow
    disp.drawRectStroke(0, PROGRESS_BAR_Y, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEGIHT);
    
    // Draw progress
    int progress = p.balance / CALIBRATION_MIN_PERCENT * (PROGRESS_BAR_WIDTH - 2);
    if (progress > (PROGRESS_BAR_WIDTH - 2)) progress = (PROGRESS_BAR_WIDTH - 2);       // 0.. 8
    disp.color = 0x00ff00; // green
    disp.drawRect(1, PROGRESS_BAR_Y + 1, progress, PROGRESS_BAR_HEGIHT - 3);

    // Draw time
    int elapsedProgress = float(p.elapsed) / CALIBRATION_MIN_TIME_MS * (PROGRESS_BAR_WIDTH - 2);
    if (elapsedProgress > (PROGRESS_BAR_WIDTH - 2)) elapsedProgress = (PROGRESS_BAR_WIDTH - 2);
    disp.color = 0x0000ff; // blue
    disp.drawRect(1, PROGRESS_BAR_Y + PROGRESS_BAR_HEGIHT - 2, elapsedProgress, 1);

    disp.update();

    return p.elapsed > CALIBRATION_MIN_TIME_MS && p.balance > CALIBRATION_MIN_PERCENT;
}

// ===== VICTORY MELODY =====
// Простая восходящая победная мелодия (как в Mario / Zelda)
void playVictoryMelody() {
    // Ноты: C5, D5, E5, G5, C6 (восходящая гамма)
    const int notes[] = {523, 587, 659, 784, 1047};
    const int noteCount = 5;
    
    for (int i = 0; i < noteCount; i++) {
        ledcSetup(BUZZER_PWM_CHANNEL, notes[i], BUZZER_PWM_RESOLUTION);
        ledcWrite(BUZZER_PWM_CHANNEL, 128);  // 50% duty cycle
        delay(BUZZER_TONE_DURATION);
        ledcWrite(BUZZER_PWM_CHANNEL, 0);
        delay(50);  // small pause between notes
    }
}

// ===== NON-BLOCKING VICTORY MELODY =====
void startVictoryMelody() {
    Serial.println("  🔊 Start playing winning melody!");
    victoryPlaying = true;
    victoryNoteIndex = 0;
    victoryStartTime = millis();
    victoryPointReached = true;  // чтобы не повторять
}

void tickBuzzer() {
    if (!victoryPlaying) return;
    
    // Ноты: C5, D5, E5, G5, C6
    const int notes[] = {523, 587, 659, 784, 1047};
    const int noteCount = 5;
    const int noteDuration = 150;
    const int pauseDuration = 50;
    
    static unsigned long lastNoteTime = 0;
    static bool playingNote = true;
    
    if (victoryNoteIndex >= noteCount) {
        // Мелодия завершена
        ledcWrite(BUZZER_PWM_CHANNEL, 0);
        victoryPlaying = false;
        Serial.println("  🔈 Winnng melody played to the end");
        return;
    }
    
    unsigned long now = millis();
    
    if (playingNote) {
        // Включаем ноту
        if (now - lastNoteTime >= noteDuration) {
            ledcWrite(BUZZER_PWM_CHANNEL, 0);
            playingNote = false;
            lastNoteTime = now;
        }
    } else {
        // Пауза между нотами
        if (now - lastNoteTime >= pauseDuration) {
            victoryNoteIndex++;
            if (victoryNoteIndex < noteCount) {
                ledcSetup(BUZZER_PWM_CHANNEL, notes[victoryNoteIndex], BUZZER_PWM_RESOLUTION);
                ledcWrite(BUZZER_PWM_CHANNEL, 128);
                playingNote = true;
                lastNoteTime = now;
            } else {
                // Мелодия завершена
                victoryPlaying = false;
            }
        }
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("\n╔═══════════════════════════════════╗");
    Serial.println("║     ESP32-C3 GPS Compass          ║");
    Serial.println("╚═══════════════════════════════════╝\n");
    
    Serial.println("STEP 0: Initializing Battery Monitor...");
    battery.init();
    Serial.println("  ✅ Battery monitor initialized");

    Serial.println("STEP 1: Initializing GPS...");
    Serial0.begin(9600, SERIAL_8N1, GPS_UART_PIN_RX, GPS_UART_PIN_TX);  // RX, TX
    Serial.printf("  ✅ GPS UART initialized on pins %d(RX), %d(TX)\n", GPS_UART_PIN_RX, GPS_UART_PIN_TX);

    Serial.println("STEP 2: Initializing File System...");
    LittleFS.begin(true);
    fdata.read();
    Serial.println("  ✅ LittleFS mounted");
    Serial.printf("  ✅ Config loaded: brightness=%d, mode=%d\n", cfg.brightness, (int)cfg.mode);

    Serial.println("STEP 3: Initializing LED Matrix...");
    strip.begin();
    strip.setBrightness(cfg.brightness);
    strip.clear();
    strip.show();
    Serial.println("  ✅ NeoPixel initialized");

    Serial.println("STEP 4: Initializing Magnetometer...");
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Wire.setClock(100000);
    Serial.printf("  ✅ I2C initialized on pins %d(SDA), %d(SCL)\n", I2C_SDA_PIN, I2C_SCL_PIN);

    // Проверяем наличие датчика
    Serial.printf("  🔍 Scanning for magnetometer at 0x%x... ", I2C_MAG_ADDR);
    Wire.beginTransmission(I2C_MAG_ADDR);
    byte error = Wire.endTransmission();
    if (error == 0) {
        Serial.println("✅ FOUND!");
    } else {
        Serial.printf("❌ NOT FOUND! Error: %d\n", error);
        Serial.printf("  Check wiring: SDA=GPIO%d, SCL=GPIO%d, VCC=3.3V, GND=GND\n", I2C_SDA_PIN, I2C_SCL_PIN);
    }
    
    // Принудительная инициализация
    Serial.print("  ⚙️  Configuring magnetometer registers... ");
    // 1. Сброс датчика (запись 0x01 в регистр 0x0B)
    Wire.beginTransmission(I2C_MAG_ADDR);
    Wire.write(0x0B);  // Register RESET
    Wire.write(0x01);  // Soft reset
    Wire.endTransmission();
    delay(10);
    // 2. Настройка Control Register (0x09)
    // Биты: [7:6] OSR (00=512 samples)
    //       [5:4] RNG (01=8G)
    //       [3:2] ODR (11=200Hz)
    //       [1:0] MODE (01=continuous mode)
    Wire.beginTransmission(I2C_MAG_ADDR);
    Wire.write(0x09);  // Control register
    Wire.write(0x1D);  // 0b00011101 = continuous mode, 200Hz, 8G, 512 samples
    Wire.endTransmission();
    delay(10);
    // 3. Настройка Set/Reset Period (0x0A)
    Wire.beginTransmission(I2C_MAG_ADDR);
    Wire.write(0x0A);  // Control2 register
    Wire.write(0x01);  // Enable set/reset
    Wire.endTransmission();
    delay(10);
    byte writeError = Wire.endTransmission();
    if (writeError == 0) {
        Serial.println("✅ Control register set to 0x1D");
    } else {
        Serial.printf("❌ Write failed! Error: %d\n", writeError);
    }
    delay(100);
    
    mag.begin();
    mag.cal = cfg.cal;
    Serial.println("  ✅ Magnetometer initialized");

    // ось y вперёд
    mag.head.axis[0] = 1;
    mag.head.axis[1] = 0;
    mag.head.sign[1] = -1;
    mag.head.declinDeg = 10;
    Serial.println("  ✅ Magnetometer axes configured");

    // ===== BUZZER INIT =====
    Serial.println("STEP 5: Initializing Buzzer...");
    ledcSetup(BUZZER_PWM_CHANNEL, 440, BUZZER_PWM_RESOLUTION);
    ledcAttachPin(BUZZER_PIN, BUZZER_PWM_CHANNEL);
    ledcWrite(BUZZER_PWM_CHANNEL, 0);
    Serial.printf("  ✅ Buzzer initialized on pin %d\n", BUZZER_PIN);

    Serial.println("\n═══════════════════════════════════════");
    Serial.println("✅ SETUP COMPLETE!");
    Serial.println("═══════════════════════════════════════\n");
    
    Serial.println("Controls:");
    Serial.println("  🔄 2 clicks - switch mode (Compass ↔ GPS)");
    Serial.println("  🔄 3 clicks - calibrate compass / save GPS point");
    Serial.println("  🔄 Hold - adjust brightness\n");
}

void loop() {
    static unsigned long lastStatus = 0;
    
    // Выводим статус раз в 5 секунд
    if (millis() - lastStatus > 5000) {
        lastStatus = millis();
        Serial.println("\n📊 STATUS UPDATE:");
        Serial.printf("  Mode: %s\n", 
                      cfg.mode == Mode::Charging ? "CHARGING" :
                      cfg.mode == Mode::Compass ? "COMPASS" :
                      cfg.mode == Mode::TargetDir ? "GPS TARGET DIR" : "GPS TARGET DIST");
        
        // Проверяем магнитометр
        MagRaw test = mag.readRaw();
        Serial.printf("  Mag RAW: X=%6d Y=%6d Z=%6d\n", test.x, test.y, test.z);
        
        // Проверяем GPS
        if (gps.location.isValid()) {
            Serial.printf("  GPS: Lat=%.6f Lon=%.6f Sats=%d\n", 
                          gps.location.lat(), gps.location.lng(), gps.satellites.value());
        } else {
            Serial.printf("  GPS: No fix (sats=%d)\n", gps.satellites.value());
        }
        
        // Проверяем кнопку
        Serial.printf("  Button state: %d\n", digitalRead(BTN_PIN));
        Serial.println("─────────────────────────────────────");
    }

    btn.tick();
    fdata.tick();

    if (Serial0.available()) {
        gps.encode(Serial0.read());
    }

    // ------ Buzzer tick (non-blocking)
    tickBuzzer();

    // ------ Display
    EVERY16_MS(100) {
        disp.clear();

        // Проверка подключения зарядки
        bool charging = digitalRead(BATTERY_CHARGING_PIN) == LOW;
        bool full = digitalRead(BATTERY_FULL_PIN) == LOW;
        Battery::State batteryState = battery.getState();
        Serial.printf("  🔋 Battery state: charging=%d, full=%d, state=%d\n", charging, full, batteryState);

        const bool batteryIsChargerConnected = battery.isChargerConnected();
        Serial.printf("  🔋 Charger connected: %d\n", batteryIsChargerConnected);
        if (batteryIsChargerConnected) { // Если зарядка подключена - переключаем в режим зарядки
            cfg.mode = Mode::Charging;
            fdata.update();
        } else { // Если нет - возвращаем в обычный, но только если была зарядка
            if (cfg.mode == Mode::Charging) {
                cfg.mode = Mode::Compass;
                fdata.update();
            }
        }

        switch (cfg.mode) {
            case Mode::Charging: {
                switch (battery.getState()) { // Проверяем состояние батарейки
                    case Battery::STATE_FULL: {
                        // Заряжена - зеленая батарейка
                        disp.color = 0x00ff00;
                        disp.drawSprite(battery_full, sizeof(battery_full), 0, 1);
                        Serial.println(" 🔋 Battery FULL");
                        break;
                    }
                    
                    case Battery::STATE_CHARGING: {
                        // Заряжается - анимация заполнения
                        static uint8_t fillLevel = 0;
                        static unsigned long lastFillUpdate = 0;
                        static const uint8_t MAX_FILL_LEVEL = 4; // Максимальный уровень заполнения
                        
                        // Обновляем уровень заполнения каждые 200 мс
                        if (millis() - lastFillUpdate > 200) {
                            lastFillUpdate = millis();
                            fillLevel++;
                            if (fillLevel > MAX_FILL_LEVEL) fillLevel = 0;
                        }
                        
                        // Рисуем корпус батарейки
                        disp.color = 0xff8800; // orange
                        disp.drawSprite(battery_frame, sizeof(battery_frame), 0, 1);
                        
                        // Рисуем заполнение от 0 до 4 пикселей
                        disp.drawRect(1, 2, fillLevel, 2);
                        
                        Serial.printf(" 🔋 Charging animation... level %d/%d\n", fillLevel, MAX_FILL_LEVEL);
                        break;
                    }
                    
                    case Battery::STATE_ERROR: {
                        // Ошибка - быстро моргаем красной батарейкой
                        static bool blinkState = false;
                        static unsigned long lastBlink = 0;
                        
                        if (millis() - lastBlink > 200) {
                            lastBlink = millis();
                            blinkState = !blinkState;
                        }
                        
                        if (blinkState) {
                            disp.color = 0xff0000;
                            disp.drawSprite(battery_frame, sizeof(battery_frame), 0, 1);
                            Serial.println(" 🔋❌ BATTERY ERROR!");
                        } else {
                            disp.clear();
                        }
                        break;
                    }
                    
                    case Battery::STATE_NOT_CHARGING: // Вообще мы не долдны сюда попадать, но на всякий случай
                    default: {
                        // Не заряжается - тусклая рамка батарейки
                        disp.color = 0x444444;
                        disp.drawSprite(battery_frame, sizeof(battery_frame), 0, 1);
                        Serial.println(" 🔋 Not charging");
                        break;
                    }
                }
                break;
            }

            case Mode::Compass: {
                float heading = mag.headingRad();
                Serial.printf("  🧭 Compass: heading=%.2f rad\n", heading);
                showArrowRad(0 - heading, compassColors);
                break;
            }

            case Mode::TargetDir:
                if (gps.location.isValid()) {
                    float headDeg = gps.courseTo(gps.location.lat(), gps.location.lng(), cfg.lat, cfg.lng);
                    float heading = mag.headingRad();
                    Serial.printf("  🎯 TargetDir: headDeg=%.2f, heading=%.2f\n", headDeg, heading);
                    showArrowRad(radians(headDeg) - heading, GPSColors);
                    float hdop = gps.hdop.hdop();
                    disp.color = Adafruit_NeoPixel::ColorHSV(hdop ? map(hdop, 20, 0, 0, 20000) : 0);
                    disp.drawPixel(5, 2);  // center
                } else {
                    Serial.println("  ❌ GPS no fix, showing cross");
                    disp.color = 0xff0000;
                    disp.drawSprite(cross, sizeof(cross));
                }
                break;

            case Mode::TargetDist:
                if (gps.location.isValid()) {
                    uint32_t dist = gps.distanceBetween(cfg.lat, cfg.lng, gps.location.lat(), gps.location.lng());
                    Serial.printf("  📏 Distance: %d m\n", dist);
                    
                    // ===== CHECK IF TARGET REACHED =====
                    if (dist <= GPS_DISTANCE_TO_POINT_REACHED_METERS) {
                        if (!victoryPointReached) { // Чтобы мелодия не играла постоянно при нахождении рядом с точкой
                            Serial.println("  🎉 TARGET POINT REACHED! Playing winning melody!");
                            startVictoryMelody();
                        }
                    } else {
                        victoryPointReached = false;
                    }
                    
                    const uint8_t BETWEEN_NUMBERS_WIDTH = 1;
                    const uint8_t PADDING_FROM_BORDER_X = (LED_MATRIX_WIDTH - BETWEEN_NUMBERS_WIDTH - SPRITE_NUMBER_WIDTH * 2) / 2;
                    const uint8_t PADDING_FROM_BORDER_Y = (LED_MATRIX_HEIGHT - SPRITE_NUMBER_HEIGHT) / 2;
                    if (dist <= 99) {
                        disp.color = 0x00ff00;
                        disp.drawNum(dist / 10, PADDING_FROM_BORDER_X, PADDING_FROM_BORDER_Y);
                        disp.color = 0x00ff00;
                        disp.drawNum(dist % 10, LED_MATRIX_WIDTH - PADDING_FROM_BORDER_X - SPRITE_NUMBER_WIDTH, PADDING_FROM_BORDER_Y);
                    // } else if (dist < 10 * LED_COUNT) {
                    } else if (dist < 10 * (7 * 7)) { // FIXME: hardcoded. Я взял матрицу 7х7 внутри матрицы 8х8
                        disp.color = 0x00ff00;
                        for (uint16_t i = 0; i < dist / 10; i++) {
                            disp.drawPixel(i % 7, i / 7);
                        }
                    // } else if (dist < 100 * LED_COUNT) {
                    } else if (dist < 100 * (7 * 7)) { // FIXME: hardcoded. Я взял матрицу 7х7 внутри матрицы 8х8
                        disp.color = 0xffff00;
                        for (uint16_t i = 0; i < dist / 100; i++) {
                            disp.drawPixel(i % 7, i / 7);
                        }
                    // } else if (dist < 1000 * LED_COUNT) {
                    } else if (dist < 1000 * (7 * 7)) { // FIXME: hardcoded. Я взял матрицу 7х7 внутри матрицы 8х8
                        disp.color = 0xff0000;
                        for (uint16_t i = 0; i < dist / 10000; i++) {
                            disp.drawPixel(i % 7, i / 7);
                        }
                    } else {
                        disp.color = 0xff0000;
                        disp.drawNum(0, PADDING_FROM_BORDER_X, PADDING_FROM_BORDER_Y);
                        disp.drawNum(0, LED_MATRIX_WIDTH - PADDING_FROM_BORDER_X - SPRITE_NUMBER_WIDTH, PADDING_FROM_BORDER_Y);
                    }
                } else {
                    Serial.println("  ❌ GPS no fix");
                    disp.color = 0xff0000;
                    disp.drawPixel(2, 2);
                    disp.drawPixel(3, 2);
                    disp.drawPixel(4, 2);
                    disp.color = 0x00ff00;
                    disp.drawPixel(5, 2);
                    disp.drawPixel(6, 2);
                    disp.drawPixel(7, 2);
                }
                break;
        }
        disp.update();
    }


    // ------ Brightness controls
    static bool dir;

    if (btn.releaseStep(0)) {
        dir = !dir;
        Serial.printf("  💡 Brightness direction: %s\n", dir ? "UP" : "DOWN");
    }
    if (btn.step()) {
        int br = cfg.brightness + (dir ? 20 : -20);
        cfg.brightness = constrain(br, 10, 255);
        strip.setBrightness(cfg.brightness);
        fdata.update();
        Serial.printf("  💡 Brightness: %d\n", cfg.brightness);
    }

    // switch mode
    if (btn.hasClicks(1)) {
        Serial.println("  👆 Single click detected");
        switch (cfg.mode) {
            case Mode::Charging:
                Serial.println("  ⏭️  Staying in CHARGING mode");
                break;

            case Mode::Compass:
                Serial.println("  ⏭️  Staying in COMPASS mode");
                break;

            case Mode::TargetDir:
                cfg.mode = Mode::TargetDist;
                fdata.update();
                Serial.println("  ⏭️  Switched to TARGET DIST mode");
                break;

            case Mode::TargetDist:
                cfg.mode = Mode::TargetDir;
                fdata.update();
                Serial.println("  ⏭️  Switched to TARGET DIR mode");
                break;
        }
    }
    if (btn.hasClicks(2)) {
        Serial.println("  👆👆 Double click detected");
        switch (cfg.mode) {
            case Mode::Charging:
                Serial.println("  ⏭️  Staying in CHARGING mode");
                break;

            case Mode::Compass:
                cfg.mode = Mode::TargetDir;
                fdata.update();
                Serial.println("  ⏭️  Switched to TARGET DIR mode");
                break;

            case Mode::TargetDir:
            case Mode::TargetDist:
                cfg.mode = Mode::Compass;
                fdata.update();
                Serial.println("  ⏭️  Switched to COMPASS mode");
                break;
        }
    }
    if (btn.hasClicks(3)) {
        Serial.println("  👆👆👆 Triple click detected!");
        switch (cfg.mode) {
            case Mode::Charging:
                Serial.println("  ⏭️  Staying in CHARGING mode");
                break;

            case Mode::Compass:
                Serial.println("  🔄 Starting compass calibration...");
                mag.calibrate(onCalibrate);
                cfg.cal = mag.cal;
                fdata.updateNow();
                Serial.println("  ✅ Calibration complete and saved!");
                break;

            case Mode::TargetDir:
            case Mode::TargetDist:
                if (gps.location.isValid()) {
                    cfg.lat = gps.location.lat();
                    cfg.lng = gps.location.lng();
                    fdata.updateNow();
                    victoryPointReached = false; // Сброс флага достижения точки при сохранении новой точки
                    Serial.printf("  📍 GPS point saved! Lat=%.6f Lon=%.6f\n", cfg.lat, cfg.lng);
                } else {
                    Serial.println("  ❌ GPS no fix, cannot save point!");
                }
                disp.clear();
                disp.color = 0x00ff00;
                disp.drawSprite(cross, sizeof(cross));
                disp.update();
                delay(500);
                break;
        }
    }
}