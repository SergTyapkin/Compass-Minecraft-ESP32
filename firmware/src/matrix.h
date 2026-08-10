#pragma once
#include <Adafruit_NeoPixel.h>

#include "arrows.h"
#include "clocks.h"
#include "config.h"
#include "gfx.h"

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

static float constrainNumberByPeriod(float num, float period) {
    // return (period + (num % period)) % period;
    while (num < 0) num += period;
    while (num >= period) num -= period;
    return num;
}
static void showSprite(size_t len, const Pix* pix, uint32_t cols[3]) {
    for (size_t i = 0; i < len; i++) {
        uint32_t color = cols[pix[i].col];
        if (color > 0) {
            strip.setPixelColor(pix[i].pos, color);
        }
    }
}
static void showArrowSprite(uint8_t n, uint32_t cols[3]) {
    const Pix* pix = getArrowPix(n);
    size_t len = getArrowLen(n);
    showSprite(len, pix, cols);
}
static void showClockSprite(uint8_t n, uint32_t cols[3]) {
    const Pix* pix = getClockPix(n);
    size_t len = getClockLen(n);
    showSprite(len, pix, cols);
}

// Показать стрелку. 0 - вперёд, в положительном направлении по часовой стрелке
static void showArrowRad(float head, uint32_t cols[3]) {
    head += COMPASS_ROTATION_OFFSET_RAD;  // доворачиваем до нужного центра
    head = constrainNumberByPeriod(head, TWO_PI); // Переводим в интервал от 0 до 2PI
    int s = head / TWO_PI * arrowAmount;
    showArrowSprite(s % arrowAmount, cols);
}
// Показать часы. Передается значение от 1 до 60
static void showClockRad(float val1to60, uint32_t cols[3]) {
    val1to60 += CLOCK_ROTATION_OFFSET_1_TO_60;  // доворачиваем до нужного центра
    val1to60 = constrainNumberByPeriod(val1to60, 60); // Переводим в интервал от 0 до 60
    int s = val1to60 / 60 * clockAmount;
    showClockSprite(s % clockAmount, cols);
}

class Disp : public GFX {
   public:
    void drawPixel(uint8_t x, uint8_t y) override {
        if (x >= LED_MATRIX_WIDTH || y >= LED_MATRIX_HEIGHT || x < 0 || y < 0) return;

        size_t pos = y * LED_MATRIX_WIDTH + x;
        strip.setPixelColor(pos, color);
    }

    void drawSprite(const uint8_t (*s)[2], size_t spriteSize, uint8_t x = 0, uint8_t y = 0) {
        size_t len = spriteSize / sizeof(s[0]);
        while (len--) drawPixel(s[len][0] + x, s[len][1] + y);
    }

    void update() {
        strip.show();
    }

    void clear() {
        strip.clear();
    }
};

Disp disp;