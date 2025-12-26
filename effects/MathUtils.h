#ifndef LED_SEGMENTS_SPECS_MATHUTILS_H
#define LED_SEGMENTS_SPECS_MATHUTILS_H

#include "FastLED.h"

// This file contains utility functions for fixed-point arithmetic and color manipulation.
// The fixed-point format used is primarily Q0.16 (unsigned, represented by fract16)
// and Q0.15 (signed, represented by int16_t).

// Q0.16 (fract16) is an unsigned 16-bit number representing a value from 0.0 to ~1.0.
// 0x0000 -> 0.0
// 0x8000 -> 0.5
// 0xFFFF -> ~1.0

// Q0.15 is a signed 16-bit number representing a value from -1.0 to ~1.0.
// -32768 -> -1.0
// 0      -> 0.0
// 32767  -> ~1.0


/**
 * @brief Scales a 16-bit unsigned integer by a Q0.16 fixed-point fraction.
 * @param value Unsigned 16-bit value to scale (0 to 65535)
 * @param scale Unsigned Q0.16 fraction (0 = 0.0, 65535 ≈ 1.0).
 * @return Scaled unsigned 16-bit result.
 */
static fl::u16 scale_u16_by_f16(fl::u16 value, fract16 scale) {
    if (scale == 0xFFFF) return value;
    return (static_cast<uint32_t>(value) * scale + 0x8000) >> 16;
}

/**
 * @brief Scales a 16-bit signed integer by a Q0.16 fixed-point fraction.
 * @param value Signed Q1.15 value to scale (-1.0 … +0.99997)
 * @param scale Unsigned Q0.16 fraction (0 = 0.0, 65535 ≈ 1.0).
 * @return Scaled signed Q1.15 result.
 */
static fl::i16 scale_i16_by_f16(fl::i16 value, fract16 scale) {
    if (scale == 0xFFFF) return value;

    int32_t result = static_cast<int32_t>(value) * static_cast<int32_t>(scale);

    // Symmetric rounding
    result += (result >= 0) ? 0x8000 : -0x8000;

    // Q0.16 × Q1.15 → Q1.31 → Q1.15
    result >>= 16;

    // Saturate to int16 range
    if (result > INT16_MAX) return INT16_MAX;
    if (result < INT16_MIN) return INT16_MIN;

    return (int16_t) result;
}

/**
 * @brief Multiplies 2 16-bit unsigned integers with saturation to 65535
 * @param left Unsigned 16-bit value (0 to 65535)
 * @param right Unsigned 16-bit value (0 to 65535)
 * @return Saturated multiplication result (0 to 65535)
 */
static fl::u16 multiply_u16_sat(fl::u16 left, fl::u16 right) {
    uint32_t product = static_cast<uint32_t>(left) * right;
    return (product > 0xFFFF) ? 0xFFFF : (fl::u16) product;
}

/**
 * @brief Divides two integers and returns the result as a Q0.16 fraction.
 * @param numerator The numerator.
 * @param denominator The denominator.
 * @return The result as a fract16 (Q0.16).
 */
static fract16 divide_u16_as_fract16(fl::u16 numerator, fl::u16 denominator) {
    if (denominator == 0) return UINT16_MAX; // Avoid division by zero

    uint32_t temp = (static_cast<uint32_t>(numerator) << 16); // Shift numerator to Q16.16 format
    // Add denominator/2 to round to nearest instead of truncating
    temp = (temp + (denominator / 2)) / denominator;

    // If numerator >= denominator, the result will be >= 0x10000.
    // Cap to UINT16_MAX to ensure it fits in a fract16.
    if (temp > UINT16_MAX) return UINT16_MAX;
    return static_cast<fract16>(temp);
}

#endif //LED_SEGMENTS_SPECS_MATHUTILS_H
