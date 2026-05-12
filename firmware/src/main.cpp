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

QMC5883L mag;

TinyGPSPlus gps;

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

bool onCalibrate(const MagCalProgress& p) {
    disp.clear();
    disp.color = 0xffff00;
    disp.rect(0, 1, 10, 3);

    int progress = p.balance / 0.75 * 8;  // 0.. 8+
    if (progress > 8) progress = 8;       // 0.. 8
    disp.color = 0x00ff00;
    for (int i = 0; i < progress; i++) {
        disp.drawPixel(1 + i, 2);
    }
    disp.update();

    // 10 секунд, покрытие 75%
    return p.elapsed > 10000 && p.balance > 0.75;
}

void setup() {
    Serial.begin(115200);                     // USB CDC
    Serial0.begin(9600, SERIAL_8N1, 20, 21);  // RX, TX

    LittleFS.begin(true);
    fdata.read();

    strip.begin();
    strip.setBrightness(cfg.brightness);

    Wire.begin();
    mag.begin();
    mag.cal = cfg.cal;

    // ось y вперёд
    mag.head.axis[0] = 1;
    mag.head.axis[1] = 0;
    mag.head.sign[1] = -1;
    mag.head.declinDeg = 10;
}

void loop() {
    // EVERY16_MS(100) {
    //     static float head;
    //     disp.clear();
    //     showArrowRad(head, compassColors);
    //     disp.update();
    //     head += 0.2;
    // }
    // return;

    btn.tick();
    fdata.tick();

    if (Serial0.available()) {
        gps.encode(Serial0.read());
    }

    // display
    EVERY16_MS(100) {
        disp.clear();
        switch (cfg.mode) {
            case Mode::Compass:
                showArrowRad(0 - mag.headingRad(), compassColors);
                break;

            case Mode::TargetDir:
                if (gps.location.isValid()) {
                    float headDeg = gps.courseTo(gps.location.lat(), gps.location.lng(), cfg.lat, cfg.lng);
                    showArrowRad(radians(headDeg) - mag.headingRad(), GPSColors);
                    float hdop = gps.hdop.hdop();
                    disp.color = Adafruit_NeoPixel::ColorHSV(hdop ? map(hdop, 20, 0, 0, 20000) : 0);
                    disp.drawPixel(5, 2);  // center
                } else {
                    disp.color = 0xff0000;
                    disp.drawSprite(cross, sizeof(cross));
                }
                break;

            case Mode::TargetDist:
                if (gps.location.isValid()) {
                    uint32_t dist = gps.distanceBetween(cfg.lat, cfg.lng, gps.location.lat(), gps.location.lng());
                    if (dist > 99) dist = 99;
                    disp.color = 0xff0000;
                    disp.printNum(dist / 10, 2, 0);

                    disp.color = 0x00ff00;
                    disp.printNum(dist % 10, 5, 0);
                } else {
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

    // bright
    static bool dir;

    if (btn.releaseStep(0)) {
        dir = !dir;
    }
    if (btn.step()) {
        int br = cfg.brightness + (dir ? 20 : -20);
        cfg.brightness = constrain(br, 10, 255);
        strip.setBrightness(cfg.brightness);
        fdata.update();
    }

    // switch mode
    if (btn.hasClicks(1)) {
        switch (cfg.mode) {
            case Mode::Compass:
                break;

            case Mode::TargetDir:
                cfg.mode = Mode::TargetDist;
                fdata.update();
                break;

            case Mode::TargetDist:
                cfg.mode = Mode::TargetDir;
                fdata.update();
                break;
        }
    }
    if (btn.hasClicks(2)) {
        switch (cfg.mode) {
            case Mode::Compass:
                cfg.mode = Mode::TargetDir;
                fdata.update();
                break;

            case Mode::TargetDir:
            case Mode::TargetDist:
                cfg.mode = Mode::Compass;
                fdata.update();
                break;
        }
    }
    if (btn.hasClicks(3)) {
        switch (cfg.mode) {
            case Mode::Compass:
                mag.calibrate(onCalibrate);
                cfg.cal = mag.cal;
                fdata.updateNow();
                break;

            case Mode::TargetDir:
            case Mode::TargetDist:
                if (gps.location.isValid()) {
                    cfg.lat = gps.location.lat();
                    cfg.lng = gps.location.lng();
                    fdata.updateNow();
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