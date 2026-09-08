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
#include "timeManager.h"
#include "medianFilter.h"

/*
    Управление кнопкой:

    Удержание:
        В любом режиме - именение яркости. При отпускании и зажатии заново меняет направление изменения яркости (вниз / вверх)
    1x клик:
        🧭 в режиме компаса - ничего
        🌐 в режиме GPS - переключение - показать расстояние до точки / направление к ней
        🕓 в реижме часов - переключение - показать время стрелкой компаса / самими часами
    2x клик - смена режима по кругу:
        => 🧭 Режим компаса -> 🌐 Режим GPS -> 🕓 Режим часов =>
    3x клик:
        🧭 в режиме компаса - калибровка компаса
        🌐 в режиме GPS - запомнить точку
        🕓 в реижме часов - синхрнизация с NPT сервером
*/

enum class Mode {
    Charging,
    Compass,
    TargetDir,
    TargetDist,
    Clock,
    ClockCompass,
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
MedianFilter<int16_t, MAG_FILTER_SIZE> magFilterX;
MedianFilter<int16_t, MAG_FILTER_SIZE> magFilterY;
MedianFilter<int16_t, MAG_FILTER_SIZE> magFilterZ;

TinyGPSPlus gps;

Battery battery(BATTERY_CHARGING_PIN, BATTERY_FULL_PIN);

TimeManager timeManager(WIFI_SSID_NAME, WIFI_PASSWORD, NTP_SERVER_HOST, GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, true);

// Цвета для вывода обычного компаса (красная стрелка)
static uint32_t DefaultCompassArrowColors[] = {
    Adafruit_NeoPixel::Color(255, 0, 0),     // Яркая стрелка
    Adafruit_NeoPixel::Color(60, 0, 0),      // Темная стрелка
    Adafruit_NeoPixel::Color(120, 120, 120), // Центр
};

// Цвета для вывода направления в GPS режиме (зеленая стрелка)
static uint32_t GPSCompassArrowColors[] = {
    Adafruit_NeoPixel::Color(0, 255, 0),     // Яркая стрелка
    Adafruit_NeoPixel::Color(0, 60, 0),      // Темная стрелка
    Adafruit_NeoPixel::Color(120, 120, 120), // Центр
};

// Цвета для вывода часов (красная стрелка)
static uint32_t HourClockCompassArrowColors[] = {
    Adafruit_NeoPixel::Color(255, 0, 0),     // Яркая стрелка
    Adafruit_NeoPixel::Color(60, 0, 0),      // Темная стрелка
    Adafruit_NeoPixel::Color(120, 120, 120), // Центр
};
// Цвета для вывода минут (синяя стрелка)
static uint32_t MinuteClockCompassArrowColors[] = {
    Adafruit_NeoPixel::Color(0, 0, 255),     // Яркая стрелка
    Adafruit_NeoPixel::Color(0, 0, 60),      // Темная стрелка
    Adafruit_NeoPixel::Color(0, 0, 0),       // Центр делаем прозрачным
};
// Цвета для вывода дисплея часов
static uint32_t ClockColors[] = {
    Adafruit_NeoPixel::Color(0, 0, 180),     // Светлое небо
    Adafruit_NeoPixel::Color(0, 0, 60),      // Темное небо
    Adafruit_NeoPixel::Color(255, 255, 0),   // Светлая часть стрелки солнца
    Adafruit_NeoPixel::Color(60, 60, 0),     // Темная часть стрелки солнца
    Adafruit_NeoPixel::Color(255, 255, 255), // Светлая часть стрелки луны
    Adafruit_NeoPixel::Color(60, 60, 60),    // Темная часть стрелки луны
};

// ===== BUZZER STATE =====
static bool victoryPointReached = false;
static unsigned long victoryStartTime = 0;
static int victoryNoteIndex = 0;
static bool victoryPlaying = false;

// ===== ANIMATIONS VARS =====
static unsigned long const INITIAL_TIME = millis();

bool onCalibrate(const MagCalProgress& progressMsg) {
    disp.clear();
    disp.color = 0xffff00; // yellow
    disp.drawRectStroke(0, PROGRESS_BAR_Y, PROGRESS_BAR_WIDTH, PROGRESS_BAR_HEGIHT);
    
    // Draw progress
    int progress = progressMsg.balance / CALIBRATION_MIN_PERCENT * (PROGRESS_BAR_WIDTH - 2);
    if (progress > (PROGRESS_BAR_WIDTH - 2)) progress = (PROGRESS_BAR_WIDTH - 2);       // 0.. 8
    disp.color = 0x00ff00; // green
    disp.drawRect(1, PROGRESS_BAR_Y + 1, progress, PROGRESS_BAR_HEGIHT - 3);

    // Draw time
    int elapsedProgress = float(progressMsg.elapsed) / CALIBRATION_MIN_TIME_MS * (PROGRESS_BAR_WIDTH - 2);
    if (elapsedProgress > (PROGRESS_BAR_WIDTH - 2)) elapsedProgress = (PROGRESS_BAR_WIDTH - 2);
    disp.color = 0x0000ff; // blue
    disp.drawRect(1, PROGRESS_BAR_Y + PROGRESS_BAR_HEGIHT - 2, elapsedProgress, 1);

    disp.update();

    return progressMsg.elapsed > CALIBRATION_MIN_TIME_MS && progressMsg.balance > CALIBRATION_MIN_PERCENT;
}

// ===== VICTORY MELODY =====
// Простая восходящая победная мелодия
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
    
    // Принудительная инициализация магнитометра
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
    mag.head.axis[0] = 0;
    mag.head.axis[1] = 1;
    mag.head.sign[0] = 1;
    mag.head.sign[1] = 1;
    mag.head.declinDeg = 10;
    Serial.println("  ✅ Magnetometer axes configured");

    // сброс фильтров
    magFilterX.reset();
    magFilterY.reset();
    magFilterZ.reset();
    Serial.printf("  ✅ Median filters initialized (size: %d)\n", MAG_FILTER_SIZE);

    // ===== BUZZER INIT =====
    Serial.println("STEP 5: Initializing Buzzer...");
    ledcSetup(BUZZER_PWM_CHANNEL, 440, BUZZER_PWM_RESOLUTION);
    ledcAttachPin(BUZZER_PIN, BUZZER_PWM_CHANNEL);
    ledcWrite(BUZZER_PWM_CHANNEL, 0);
    Serial.printf("  ✅ Buzzer initialized on pin %d\n", BUZZER_PIN);

    // ===== TimeManager INIT =====
    Serial.println("STEP 6: Initializing TimeManager...");
    timeManager.begin();
    Serial.println("  ✅ TimeManager initialized succcessfully");

    // ----- Завершение инициализации
    Serial.println("\n═══════════════════════════════════════");
    Serial.println("✅ SETUP COMPLETED!");
    Serial.println("═══════════════════════════════════════\n");
    
    Serial.println("Controls:");
    Serial.println("  🔄 1 click:");
    Serial.println("    🧭 Compass - nothing");
    Serial.println("    🌐 GPS -     switch target/dist mode");
    Serial.println("    🕓 Clock -   switch compass/clock visual mode");
    Serial.println("  🔄 2 clicks:");
    Serial.println("     Switch mode > 🧭 Compass -> 🌐 GPS -> 🕓 Clock >");
    Serial.println("  🔄 3 clicks:");
    Serial.println("    🧭 Calibrate compass");
    Serial.println("    🌐 Save GPS point");
    Serial.println("    🕓 Sync clock using WiFi");
    Serial.println("  🔄 Hold - adjust brightness\n");
}

void loop() {
    btn.tick();
    fdata.tick();
    tickBuzzer();

    if (Serial0.available()) {
        gps.encode(Serial0.read());
    }

    // ------ Display
    EVERY16_MS(30) { // 30 ms for 30+ fps
        disp.clear();

        Serial.println("\n💠 System monitor info:");
        Serial.printf("  🔆 Brightness: %d / 255\n", cfg.brightness);
        // Проверка подключения зарядки
        Battery::State batteryState = battery.getState();
        Serial.printf(
            "  🔋 Battery state: %s\n", 
            batteryState == Battery::State::STATE_CHARGING ? "🟡 Charging" :
            batteryState == Battery::State::STATE_FULL ? "🟢 Full" :
            batteryState == Battery::State::STATE_ERROR ? "🔴 Error" :
            batteryState == Battery::State::STATE_NOT_CHARGING ? "🔻 OK" :
            "❓ Unknown"
        );

        // Проверка пинов на всякий случай
        const bool batteryIsChargerConnected = battery.isChargerConnected();
        bool chargingPinValue = digitalRead(BATTERY_CHARGING_PIN) == LOW;
        bool fullPinValue = digitalRead(BATTERY_FULL_PIN) == LOW;
        Serial.printf("  🔋 Battery charger connected: %s\n", batteryIsChargerConnected ? "🟢" : "🔸");
        Serial.printf("  🔋 Charging pin:              %s\n", chargingPinValue ? "🟢" : "🔸");
        Serial.printf("  🔋 Full pin:                  %s\n", fullPinValue ? "🟢" : "🔸");
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
                Serial.println("💠 Mode: 🔋 Charging");
                switch (battery.getState()) { // Проверяем состояние батарейки
                    case Battery::STATE_FULL: {
                        // ===== РИСУЕМ БАТАРЕЙКУ С ПЛАВНЫМ ПЕРЕЛИВОМ =====
                        unsigned long timeFromStart = millis() - INITIAL_TIME;
                        
                        // Получаем оттенок цвета
                        uint16_t hue = 
                            ANIMATION_BATTERY_FULL_HUE_INITIAL_DEG * 256 +
                            sin(timeFromStart * ANIMATION_BATTERY_FULL_HUE_ANIMATION_SPEED) *
                            ANIMATION_BATTERY_FULL_HUE_AMPLITUDE_DEG * 256;
                        uint32_t frameColor = strip.ColorHSV(hue, 255, ANIMATION_BATTERY_FULL_HUE_BRIGHTNESS);
                        
                        // Рисуем батарейку с плавно меняющимся цветом
                        disp.color = frameColor;
                        disp.drawSprite(battery_full, sizeof(battery_full), 0, 1);
                        Serial.println(" 🔋 Battery FULL");
                        break;
                    }
                    
                    case Battery::STATE_CHARGING: {
                        // ===== 1. РИСУЕМ КОРПУС БАТАРЕЙКИ С ПЛАВНЫМ ПЕРЕЛИВОМ =====
                        unsigned long timeFromStart = millis() - INITIAL_TIME;
                        
                        // Получаем оттенок цвета
                        uint16_t hue = 
                            ANIMATION_BATTERY_HUE_INITIAL_DEG * 256 +
                            sin(timeFromStart * ANIMATION_BATTERY_HUE_ANIMATION_SPEED) *
                            ANIMATION_BATTERY_HUE_AMPLITUDE_DEG * 256;
                        uint32_t frameColor = strip.ColorHSV(hue, 255, ANIMATION_BATTERY_HUE_BRIGHTNESS);
                        
                        // Рисуем корпус батарейки с плавно меняющимся цветом
                        disp.color = frameColor;
                        disp.drawSprite(battery_frame, sizeof(battery_frame), 0, 1);
                        
                        // ===== 2. АНИМАЦИЯ ЗАПОЛНЕНИЯ ПОЛОСЫ ЗАРЯДКИ ГРАДИЕНТНОЙ ВОЛНОЙ =====
                        static unsigned long lastAnimUpdate = 0;
                        static int wavePosition = -ANIMATION_WAVE_WIDTH;
                        
                        // Обновляем анимацию (волна шагает)
                        if (millis() - lastAnimUpdate > ANIMATION_WAVE_STEP_MS) {
                            lastAnimUpdate = millis();
                            
                            // Фаза волны
                            wavePosition++;
                            if (wavePosition > ANIMATION_WAVE_MAX_POSITION) {
                                wavePosition = -ANIMATION_WAVE_WIDTH;
                            }
                        }
                        
                        // Определяем яркость каждого пикселя в зависимости от позиции волны
                        // Используем 5x2 область полосы для заполнения (x = 1..5, y = 2..3)
                        for (int y = 2; y <= 3; y++) {
                            for (int x = 1; x <= 5; x++) {
                                // Вычисляем расстояние от текущей позиции до центра волны
                                int distFromWave = abs(x - wavePosition);
                                
                                // Яркость зависит от расстояния до центра волны:
                                uint8_t brightness = 0;
                                // Ремаппинг расстояния в яркость (0-max) -> (255-0)
                                brightness = map(distFromWave, 0, ANIMATION_WAVE_WIDTH + 0.01, ANIMATION_WAVE_MAX_BRIGHTNESS, 0);
                                
                                if (distFromWave > ANIMATION_WAVE_WIDTH) {
                                    continue;
                                }

                                uint32_t waveColor = strip.ColorHSV(ANIMATION_WAVE_HUE_DEG * 256, 255, brightness);
                                disp.color = waveColor;
                                disp.drawPixel(x, y);
                            }
                        }
                        break;
                        Serial.println(" 🔋 Battery in charging... Playing wave animation");
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
                // ===== ЧТЕНИЕ С ФИЛЬТРАЦИЕЙ =====
                MagRaw rawValues = mag.readRaw();
                
                // Добавляем в фильтры
                magFilterX.add(rawValues.x);
                magFilterY.add(rawValues.y);
                magFilterZ.add(rawValues.z);
                
                // Получаем отфильтрованные значения
                int16_t filteredX, filteredY, filteredZ;
                if (magFilterX.isFull()) {
                    filteredX = magFilterX.getMedian();
                    filteredY = magFilterY.getMedian();
                    filteredZ = magFilterZ.getMedian();
                } else {
                    // Пока фильтр не заполнен, используем сырые данные
                    filteredX = rawValues.x;
                    filteredY = rawValues.y;
                    filteredZ = rawValues.z;
                }
                
                // Вычисляем heading из отфильтрованных данных
                float heading = atan2(filteredY, filteredX);
                if (heading < 0) heading += TWO_PI;
                
                Serial.println("💠 Mode: 🧭 Compass");
                Serial.printf("  🧭 RAW values: X=%6d Y=%6d Z=%6d\n", rawValues.x, rawValues.y, rawValues.z);
                Serial.printf("  🧭 FILT values: X=%6d Y=%6d Z=%6d\n", filteredX, filteredY, filteredZ);
                Serial.printf("  🧭 Heading=%.2f rad (%.1f°)\n", heading, degrees(heading));
                
                // Выводим стрелку компаса
                showArrowRad(COMPASS_ROTATION_OFFSET_RAD - heading, DefaultCompassArrowColors);
                break;
            }

            case Mode::TargetDir:
                Serial.println("💠 Mode: 🌐 GPS target to DIR");

                if (gps.location.isValid()) {
                    Serial.printf("  🌐 GPS: Lat=%.6f Lon=%.6f Sats=%d\n", gps.location.lat(), gps.location.lng(), gps.satellites.value());
                    float headDeg = gps.courseTo(gps.location.lat(), gps.location.lng(), cfg.lat, cfg.lng);
                    float heading = mag.headingRad();
                    Serial.printf("  🌐 TargetDir: headDeg=%.2f, heading=%.2f\n", headDeg, heading);
                    showArrowRad(radians(headDeg) - heading, GPSCompassArrowColors);
                    float hdop = gps.hdop.hdop();
                    disp.color = Adafruit_NeoPixel::ColorHSV(hdop ? map(hdop, 20, 0, 0, 20000) : 0);
                    disp.drawPixel(5, 2);  // center
                } else {
                    Serial.printf("  🌐 GPS: ❌ No fix (satellites=%d)\n", gps.satellites.value());
                    disp.color = 0xff0000;
                    disp.drawSprite(cross, sizeof(cross));
                }
                break;

            case Mode::TargetDist:
                Serial.println("💠 Mode: 🌐 GPS DIST to target");

                if (gps.location.isValid()) {
                    Serial.printf("  🌐 GPS: Lat=%.6f Lon=%.6f Sats=%d\n", gps.location.lat(), gps.location.lng(), gps.satellites.value());
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
                    } else if (dist < 10 * (GPS_MATRIX_WIDTH * GPS_MATRIX_HEIGHT)) {
                        disp.color = 0x00ff00;
                        for (uint16_t i = 0; i < dist / 10; i++) {
                            disp.drawPixel(i % GPS_MATRIX_WIDTH, i / GPS_MATRIX_HEIGHT);
                        }
                    } else if (dist < 100 * (GPS_MATRIX_WIDTH * GPS_MATRIX_HEIGHT)) {
                        disp.color = 0xffff00;
                        for (uint16_t i = 0; i < dist / 100; i++) {
                            disp.drawPixel(i % GPS_MATRIX_WIDTH, i / GPS_MATRIX_HEIGHT);
                        }
                    } else if (dist < 1000 * (GPS_MATRIX_WIDTH * GPS_MATRIX_HEIGHT)) {
                        disp.color = 0xff0000;
                        for (uint16_t i = 0; i < dist / 10000; i++) {
                            disp.drawPixel(i % GPS_MATRIX_WIDTH, i / GPS_MATRIX_HEIGHT);
                        }
                    } else {
                        disp.color = 0xff0000;
                        disp.drawNum(0, PADDING_FROM_BORDER_X, PADDING_FROM_BORDER_Y);
                        disp.drawNum(0, LED_MATRIX_WIDTH - PADDING_FROM_BORDER_X - SPRITE_NUMBER_WIDTH, PADDING_FROM_BORDER_Y);
                    }
                } else {
                    Serial.printf("  🌐 GPS: ❌ No fix (satellites=%d)\n", gps.satellites.value());
                    disp.color = 0xff0000;
                    disp.drawSprite(cross, sizeof(cross));
                }
                break;

            case Mode::ClockCompass: {
                Serial.println("💠 Mode: 🕓 Clock as arrows");
                Serial.printf("  🕓 Full Datetime: 📆 ");
                Serial.println(timeManager.getDateTimeString());
                Serial.printf("  🌐 NTP Synced:    %s\n", timeManager.getIsNtpSynced() ? "✅ Yes" : "❌ No");
                
                struct tm timeinfo = timeManager.getLocalTimeStruct();
                // Выводим часовую стрелку компаса
                Serial.printf("  ⌛ Hours:         %d\n", timeinfo.tm_hour);
                showArrowRad(float(timeinfo.tm_hour % 12) / 12.0 * TWO_PI, HourClockCompassArrowColors);
                // Выводим минутную стрелку компаса
                Serial.printf("  ⌛ Minutes:       %d\n", timeinfo.tm_min);
                showArrowRad(float(timeinfo.tm_min) / 60.0 * TWO_PI, MinuteClockCompassArrowColors);
                // Выводим 12 желтых квадратиков по сторонам (по 3 на каждой)
                // Делаем яркость в 2 раза меньше
                strip.setBrightness(max(cfg.brightness / 2, BRIGHTNESS_MIN));
                float dWidth = float(CLOCK_MATRIX_WIDTH) / 4.0;
                float dHeight = float(CLOCK_MATRIX_HEIGHT) / 4.0;
                disp.color = 0xffff00;
                for (uint8_t i = 1; i <= 3; i++) {
                    disp.drawPixel(CLOCK_MATRIX_X + int(dWidth * i), CLOCK_MATRIX_Y); // Верх
                    disp.drawPixel(CLOCK_MATRIX_X + int(dWidth * i), CLOCK_MATRIX_Y + CLOCK_MATRIX_HEIGHT - 1); // Низ
                    disp.drawPixel(CLOCK_MATRIX_X, CLOCK_MATRIX_Y + int(dHeight * i)); // Лево
                    disp.drawPixel(CLOCK_MATRIX_X + CLOCK_MATRIX_WIDTH - 1, CLOCK_MATRIX_Y + int(dHeight * i)); // Право
                }
                // Выводим оранжевый квадратик на 12 часах сверху
                disp.color = 0xff5500;
                disp.drawPixel(CLOCK_MATRIX_X + CLOCK_MATRIX_WIDTH / 2, CLOCK_MATRIX_Y);
                // Возвращаем яркость как было
                strip.setBrightness(cfg.brightness);
                break;
            }

            case Mode::Clock: {
                Serial.println("💠 Mode: 🕓 Clock display");
                Serial.printf("  🕓 Full Datetime: 📆 ");
                Serial.println(timeManager.getDateTimeString());
                Serial.printf("  🌐 NTP Synced:    %s\n", timeManager.getIsNtpSynced() ? "✅ Yes" : "❌ No");
                
                time_t secondsTotal = timeManager.getTime() + GMT_OFFSET_SEC;
                // Ориентируемся на секунды в дне, и по ним показываем картинку
                Serial.printf("  ⌛ Seconds total:      %d\n", secondsTotal);
                uint32_t maxSecondsInDay = 60 * 60 * 24;
                uint32_t secondsInDay = secondsTotal % maxSecondsInDay;
                Serial.printf("  ⌛ Seconds in day:     %d / %d\n", secondsInDay, maxSecondsInDay);
                showClockRad(float(secondsInDay) / float(maxSecondsInDay) * 60.0, ClockColors);
                break;
            }
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
        int br = cfg.brightness + (dir ? BRIGHTNESS_STEP : -BRIGHTNESS_STEP);
        cfg.brightness = constrain(br, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
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

            case Mode::Clock:
                cfg.mode = Mode::ClockCompass;
                fdata.update();
                Serial.println("  ⏭️  Switched to CLOCK COMPASS mode");
                break;
            case Mode::ClockCompass:
                cfg.mode = Mode::Clock;
                fdata.update();
                Serial.println("  ⏭️  Switched to CLOCK mode");
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
                cfg.mode = Mode::Clock;
                fdata.update();
                Serial.println("  ⏭️  Switched to CLOCK mode");
                break;

            case Mode::Clock:
            case Mode::ClockCompass:
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

            case Mode::Clock:
            case Mode::ClockCompass:
                Serial.println("  🔄 Starting clocks NTP syncing...");
                if (timeManager.forceSync()) {
                    Serial.println("  ✅ Clocks NTP synced!");
                } else {
                    Serial.println("  ❌ Clocks NTP syncronization failed");
                }
                break;
        }
    }
}