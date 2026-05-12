#pragma once
#include <Adafruit_NeoPixel.h>

#include "arrows.h"
#include "config.h"
#include "gfx.h"

Adafruit_NeoPixel strip(LED_N, LED_PIN, NEO_GRB + NEO_KHZ800);

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
        if (x > 9 || y > 4) return;
        if (y == 4 && (x == 0 || x == 1 || x == 8 || x == 9)) return;

        size_t pos = y * 10 + ((y & 1) ? (9 - x) : x);
        if (y == 4) pos -= 2;
        strip.setPixelColor(pos, color);
    }

    void drawSprite(const uint8_t (*s)[2], size_t size) {
        size_t len = size / sizeof(s[0]);
        while (len--) drawPixel(s[len][0], s[len][1]);
    }

    void update() {
        strip.show();
    }

    void clear() {
        strip.clear();
    }
};

Disp disp;