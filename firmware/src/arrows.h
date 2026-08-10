#pragma once
#include "pixel.h"

#define AR_RED 0   // Светлая часть стрелки
#define AR_RED_D 1 // Темная часть стрелки
#define AR_GRAY 2  // Центр

static const Pix AR_0[] = {
    {AR_GRAY, 19},
    {AR_GRAY, 26},
    {AR_GRAY, 28},
    {AR_RED, 27},
    {AR_RED, 35},
    {AR_RED, 43},
    {AR_RED, 51},
};
static const Pix AR_1[] = {
    {AR_GRAY, 19},
    {AR_GRAY, 26},
    {AR_GRAY, 28},
    {AR_RED_D, 43},
    {AR_RED, 27},
    {AR_RED, 35},
    {AR_RED, 42},
    {AR_RED, 50},
};
static const Pix AR_2[] = {
    {AR_GRAY, 19},
    {AR_GRAY, 20},
    {AR_GRAY, 26},
    {AR_GRAY, 28},
    {AR_RED_D, 35},
    {AR_RED_D, 50},
    {AR_RED, 27},
    {AR_RED, 34},
    {AR_RED, 42},
    {AR_RED, 49},
};
static const Pix AR_3[] = {
    {AR_GRAY, 20},
    {AR_GRAY, 28},
    {AR_RED_D, 35},
    {AR_RED_D, 42},
    {AR_RED_D, 49},
    {AR_RED, 27},
    {AR_RED, 34},
    {AR_RED, 41},
    {AR_RED, 48},
};
static const Pix AR_4[] = {
    {AR_GRAY, 20},
    {AR_GRAY, 21},
    {AR_GRAY, 28},
    {AR_RED_D, 35},
    {AR_RED_D, 42},
    {AR_RED, 27},
    {AR_RED, 34},
    {AR_RED, 40},
    {AR_RED, 41},
};
static const Pix AR_5[] = {
    {AR_GRAY, 20},
    {AR_GRAY, 21},
    {AR_GRAY, 28},
    {AR_RED_D, 26},
    {AR_RED, 27},
    {AR_RED, 33},
    {AR_RED, 34},
    {AR_RED, 40},
};
static const Pix AR_6[] = {
    {AR_GRAY, 19},
    {AR_GRAY, 28},
    {AR_GRAY, 29},
    {AR_GRAY, 35},
    {AR_RED_D, 34},
    {AR_RED, 26},
    {AR_RED, 27},
    {AR_RED, 32},
    {AR_RED, 33},
};
static const Pix AR_7[] = {
    {AR_GRAY, 19},
    {AR_GRAY, 28},
    {AR_GRAY, 29},
    {AR_GRAY, 35},
    {AR_RED, 24},
    {AR_RED, 25},
    {AR_RED, 26},
    {AR_RED, 27},
};
static const Pix AR_8[] = {
    {AR_GRAY, 19},
    {AR_GRAY, 28},
    {AR_GRAY, 35},
    {AR_RED_D, 25},
    {AR_RED, 16},
    {AR_RED, 17},
    {AR_RED, 26},
    {AR_RED, 27},
};
static const Pix AR_9[] = {
    {AR_GRAY, 28},
    {AR_GRAY, 35},
    {AR_GRAY, 36},
    {AR_RED_D, 16},
    {AR_RED_D, 26},
    {AR_RED, 8},
    {AR_RED, 17},
    {AR_RED, 18},
    {AR_RED, 27},
};
static const Pix AR_10[] = {
    {AR_GRAY, 35},
    {AR_GRAY, 36},
    {AR_RED_D, 8},
    {AR_RED_D, 17},
    {AR_RED_D, 26},
    {AR_RED, 0},
    {AR_RED, 9},
    {AR_RED, 18},
    {AR_RED, 27},
};
static const Pix AR_11[] = {
    {AR_GRAY, 35},
    {AR_GRAY, 36},
    {AR_GRAY, 44},
    {AR_RED_D, 17},
    {AR_RED_D, 26},
    {AR_RED, 1},
    {AR_RED, 9},
    {AR_RED, 18},
    {AR_RED, 27},
};
static const Pix AR_12[] = {
    {AR_GRAY, 35},
    {AR_GRAY, 36},
    {AR_GRAY, 44},
    {AR_RED_D, 19},
    {AR_RED, 1},
    {AR_RED, 10},
    {AR_RED, 18},
    {AR_RED, 27},
};
static const Pix AR_13[] = {
    {AR_GRAY, 26},
    {AR_GRAY, 28},
    {AR_GRAY, 35},
    {AR_RED_D, 18},
    {AR_RED, 2},
    {AR_RED, 10},
    {AR_RED, 19},
    {AR_RED, 27},
};
static const Pix AR_14[] = {
    {AR_GRAY, 26},
    {AR_GRAY, 28},
    {AR_GRAY, 35},
    {AR_RED, 3},
    {AR_RED, 11},
    {AR_RED, 19},
    {AR_RED, 27},
};
static const Pix AR_15[] = {
    {AR_GRAY, 26},
    {AR_GRAY, 28},
    {AR_GRAY, 35},
    {AR_RED_D, 11},
    {AR_RED, 4},
    {AR_RED, 12},
    {AR_RED, 19},
    {AR_RED, 27},
};
static const Pix AR_16[] = {
    {AR_GRAY, 26},
    {AR_GRAY, 34},
    {AR_GRAY, 35},
    {AR_RED_D, 4},
    {AR_RED_D, 19},
    {AR_RED, 6},
    {AR_RED, 5},
    {AR_RED, 12},
    {AR_RED, 20},
    {AR_RED, 27},
};
static const Pix AR_17[] = {
    {AR_GRAY, 26},
    {AR_GRAY, 34},
    {AR_RED_D, 5},
    {AR_RED_D, 12},
    {AR_RED_D, 19},
    {AR_RED, 6},
    {AR_RED, 13},
    {AR_RED, 20},
    {AR_RED, 27},
};
static const Pix AR_18[] = {
    {AR_GRAY, 26},
    {AR_GRAY, 33},
    {AR_GRAY, 34},
    {AR_RED_D, 12},
    {AR_RED_D, 19},
    {AR_RED, 13},
    {AR_RED, 14},
    {AR_RED, 20},
    {AR_RED, 27},
};
static const Pix AR_19[] = {
    {AR_GRAY, 26},
    {AR_GRAY, 33},
    {AR_GRAY, 34},
    {AR_RED_D, 28},
    {AR_RED, 14},
    {AR_RED, 20},
    {AR_RED, 21},
    {AR_RED, 27},
};
static const Pix AR_20[] = {
    {AR_GRAY, 19},
    {AR_GRAY, 25},
    {AR_GRAY, 26},
    {AR_GRAY, 35},
    {AR_RED_D, 20},
    {AR_RED, 21},
    {AR_RED, 22},
    {AR_RED, 27},
    {AR_RED, 28},
};
static const Pix AR_21[] = {
    {AR_GRAY, 19},
    {AR_GRAY, 25},
    {AR_GRAY, 26},
    {AR_GRAY, 35},
    {AR_RED, 27},
    {AR_RED, 28},
    {AR_RED, 29},
    {AR_RED, 30},
};
static const Pix AR_22[] = {
    {AR_GRAY, 19},
    {AR_GRAY, 26},
    {AR_RED_D, 29},
    {AR_RED, 27},
    {AR_RED, 28},
    {AR_RED, 37},
    {AR_RED, 38},
};
static const Pix AR_23[] = {
    {AR_GRAY, 18},
    {AR_GRAY, 19},
    {AR_GRAY, 26},
    {AR_RED_D, 28},
    {AR_RED_D, 38},
    {AR_RED, 27},
    {AR_RED, 36},
    {AR_RED, 37},
    {AR_RED, 46},
};
static const Pix AR_24[] = {
    {AR_GRAY, 18},
    {AR_GRAY, 19},
    {AR_RED_D, 28},
    {AR_RED_D, 37},
    {AR_RED_D, 46},
    {AR_RED, 27},
    {AR_RED, 36},
    {AR_RED, 45},
    {AR_RED, 54},
};
static const Pix AR_25[] = {
    {AR_GRAY, 10},
    {AR_GRAY, 18},
    {AR_GRAY, 19},
    {AR_RED_D, 28},
    {AR_RED_D, 37},
    {AR_RED, 27},
    {AR_RED, 36},
    {AR_RED, 45},
    {AR_RED, 53},
};
static const Pix AR_26[] = {
    {AR_GRAY, 10},
    {AR_GRAY, 18},
    {AR_GRAY, 19},
    {AR_RED_D, 35},
    {AR_RED, 27},
    {AR_RED, 36},
    {AR_RED, 44},
    {AR_RED, 53},
};
static const Pix AR_27[] = {
    {AR_GRAY, 19},
    {AR_GRAY, 26},
    {AR_GRAY, 28},
    {AR_RED_D, 36},
    {AR_RED, 27},
    {AR_RED, 35},
    {AR_RED, 44},
    {AR_RED, 52},
};
static const Pix* const AR_FRN[] = {
    AR_0,
    AR_1,
    AR_2,
    AR_3,
    AR_4,
    AR_5,
    AR_6,
    AR_7,
    AR_8,
    AR_9,
    AR_10,
    AR_11,
    AR_12,
    AR_13,
    AR_14,
    AR_15,
    AR_16,
    AR_17,
    AR_18,
    AR_19,
    AR_20,
    AR_21,
    AR_22,
    AR_23,
    AR_24,
    AR_25,
    AR_26,
    AR_27,
};
static const uint8_t AR_SZ[] = {
    sizeof(AR_0),
    sizeof(AR_1),
    sizeof(AR_2),
    sizeof(AR_3),
    sizeof(AR_4),
    sizeof(AR_5),
    sizeof(AR_6),
    sizeof(AR_7),
    sizeof(AR_8),
    sizeof(AR_9),
    sizeof(AR_10),
    sizeof(AR_11),
    sizeof(AR_12),
    sizeof(AR_13),
    sizeof(AR_14),
    sizeof(AR_15),
    sizeof(AR_16),
    sizeof(AR_17),
    sizeof(AR_18),
    sizeof(AR_19),
    sizeof(AR_20),
    sizeof(AR_21),
    sizeof(AR_22),
    sizeof(AR_23),
    sizeof(AR_24),
    sizeof(AR_25),
    sizeof(AR_26),
    sizeof(AR_27),
};

uint8_t getArrowLen(uint8_t n) {
    return AR_SZ[n] / sizeof(Pix);
}

const Pix* getArrowPix(uint8_t n) {
    return AR_FRN[n];
}

const uint8_t arrowAmount = sizeof(AR_SZ) / sizeof(AR_SZ[0]);