#pragma once
#include <stdint.h>

struct Pix {
    uint8_t col;
    uint8_t pos;
};

const Pix* getPix(uint8_t n);
uint8_t getArrowLen(uint8_t n);
extern const uint8_t arrowAmount;