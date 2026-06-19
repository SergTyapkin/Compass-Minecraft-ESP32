#pragma once
#include <Adafruit_NeoPixel.h>

#include "arrows.h"
#include "config.h"
#include "gfx.h"

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

static void showArrowSprite(uint8_t n, uint32_t cols[3]) {
    const Pix* pix = getPix(n);
    size_t len = getArrowLen(n);

    for (size_t i = 0; i < len; i++) {
        strip.setPixelColor(pix[i].pos, cols[pix[i].col]);
    }
}

// показать стрелку. 0 - вперёд, в положительном направлении по часовой стрелке
static void showArrowRad(float head, uint32_t cols[3]) {
    head += PI;  // повернуть на 180
    while (head < 0) head += TWO_PI;
    while (head >= TWO_PI) head -= TWO_PI;
    int s = head / TWO_PI * arrowAmount;
    showArrowSprite(s % arrowAmount, cols);
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