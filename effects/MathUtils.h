#ifndef LED_SEGMENTS_SPECS_MATHUTILS_H
#define LED_SEGMENTS_SPECS_MATHUTILS_H

#include "FastLED.h"

// Q0.16 multiply: unsigned
static uint16_t qmul16u(uint16_t a, uint16_t b) {
    uint32_t temp = (uint32_t) a * (uint32_t) b;
    temp = (temp + 0x8000) >> 16;
    return (uint16_t) temp;
}

// Q0.16 scale: multiply with shift 16
static uint16_t scale16u(uint16_t value, uint16_t scale) {
    return qmul16u(value, scale);
}

#endif //LED_SEGMENTS_SPECS_MATHUTILS_H
