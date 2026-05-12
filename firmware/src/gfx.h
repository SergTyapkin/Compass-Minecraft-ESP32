#pragma once
#include <Arduino.h>

#include "numbers.h"

class GFX {
   public:
    GFX() {}

    virtual void drawPixel(uint8_t x, uint8_t y) = 0;

    void rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
        for (uint8_t i = 0; i < w; i++) {
            drawPixel(x + i, y);
            drawPixel(x + i, y + h - 1);
        }
        for (uint8_t i = 0; i < h; i++) {
            drawPixel(x, y + i);
            drawPixel(x + w - 1, y + i);
        }
    }

    void bitmap(uint8_t x, uint8_t y, const uint8_t* data, uint8_t w, uint8_t h) {
        for (uint8_t j = 0; j < h; j++) {
            for (uint8_t i = 0; i < w; i++) {
                if (data[j * w + i]) drawPixel(x + i, y + j);
            }
        }
    }

    void printNum(uint8_t num, uint8_t x, uint8_t y) {
        if (num > 9) return;
        bitmap(x, y, (const uint8_t*)_numbers[num], 3, 5);
    }

    uint32_t color = 0;
};