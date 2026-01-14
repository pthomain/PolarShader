//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2023 Pierre Thomain

/*
 * This file is part of LED Segments.
 *
 * LED Segments is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LED Segments is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LED Segments. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LED_SEGMENTS_SPECS_MATHUTILS_H
#define LED_SEGMENTS_SPECS_MATHUTILS_H

#include "FastLED.h"

namespace LEDSegments {
    const static fl::u16 U16_HALF = 0x8000;

    static fl::u16 scale_u16_by_f16(fl::u16 value, fract16 scale) {
        if (scale == 0xFFFF) return value;
        uint32_t result = static_cast<uint32_t>(value) * scale;
        result += 0x8000;
        result >>= 16;
        return (fl::u16) result;
    }

    static fl::i16 scale_i16_by_f16(fl::i16 value, fract16 scale) {
        if (scale == 0xFFFF) return value;
        int32_t result = static_cast<int32_t>(value) * static_cast<int32_t>(scale);
        result += (result >= 0) ? 0x8000 : -0x8000;
        result >>= 16;
        if (result > INT16_MAX) return INT16_MAX;
        if (result < INT16_MIN) return INT16_MIN;
        return (int16_t) result;
    }

    static fl::i32 scale_i32_by_f16(fl::i32 value, fract16 scale) {
        if (scale == 0xFFFF) return value;
        int64_t result = static_cast<int64_t>(value) * static_cast<int64_t>(scale);
        result += (result >= 0) ? 0x8000 : -0x8000;
        result >>= 16;
        if (result > INT32_MAX) return INT32_MAX;
        if (result < INT32_MIN) return INT32_MIN;
        return (int32_t) result;
    }

    static fl::i32 scale_i32_by_q16_16(fl::i32 value, int32_t scale) {
        if (scale == 0) return 0;
        if (scale == (1 << 16)) return value;
        int64_t result = static_cast<int64_t>(value) * static_cast<int64_t>(scale);
        result += (result >= 0) ? 0x8000 : -0x8000;
        result >>= 16;
        if (result > INT32_MAX) return INT32_MAX;
        if (result < INT32_MIN) return INT32_MIN;
        return (int32_t) result;
    }

    /**
     * @brief Integer square root for 32-bit values.
     * @param value The unsigned value to square-root.
     * @return floor(sqrt(value)) clamped to UINT16_MAX.
     */
    static uint16_t sqrt_u32(uint32_t value) {
        uint32_t op = value;
        uint32_t res = 0;
        uint32_t one = 1uL << 30;

        while (one > op) {
            one >>= 2;
        }

        while (one != 0) {
            if (op >= res + one) {
                op -= res + one;
                res = (res >> 1) + one;
            } else {
                res >>= 1;
            }
            one >>= 2;
        }

        if (res > UINT16_MAX) return UINT16_MAX;
        return static_cast<uint16_t>(res);
    }

    static fl::u16 multiply_u16_sat(fl::u16 left, fl::u16 right) {
        uint32_t product = static_cast<uint32_t>(left) * right;
        return (product > 0xFFFF) ? 0xFFFF : (fl::u16) product;
    }

    static fract16 divide_u16_as_fract16(fl::u16 numerator, fl::u16 denominator) {
        if (denominator == 0) return UINT16_MAX;
        uint32_t temp = (static_cast<uint32_t>(numerator) << 16);
        temp = (temp + (denominator / 2)) / denominator;
        if (temp > UINT16_MAX) return UINT16_MAX;
        return static_cast<fract16>(temp);
    }

    /**
     * @brief Fixed-point atan2 approximation that returns turns (0..65535).
     * @param y Signed Y coordinate (Q15.0).
     * @param x Signed X coordinate (Q15.0).
     * @return Angle in turns, where 65536 represents 1 full rotation.
     */
    static fl::u16 atan2_turns_approx(int16_t y, int16_t x) {
        if (x == 0 && y == 0) return 0;

        uint16_t abs_x = (x < 0) ? static_cast<uint16_t>(-x) : static_cast<uint16_t>(x);
        uint16_t abs_y = (y < 0) ? static_cast<uint16_t>(-y) : static_cast<uint16_t>(y);

        uint16_t max_val = (abs_x > abs_y) ? abs_x : abs_y;
        uint16_t min_val = (abs_x > abs_y) ? abs_y : abs_x;

        uint32_t z = (static_cast<uint32_t>(min_val) << 16) / max_val; // Q0.16
        uint32_t one_minus_z = 65536u - z;

        static constexpr uint32_t A_Q16 = 8192u; // 0.125 turns in Q0.16
        static constexpr uint32_t B_Q16 = 2847u; // 0.04345 turns in Q0.16

        uint32_t inner = A_Q16 + ((B_Q16 * one_minus_z) >> 16);
        uint32_t base = (z * inner) >> 16; // 0..0.125 turns

        uint32_t angle = (abs_x >= abs_y) ? base : (16384u - base);
        if (x < 0) {
            angle = 32768u - angle;
        }
        if (y < 0) {
            angle = 65536u - angle;
        }

        return static_cast<fl::u16>(angle & 0xFFFFu);
    }

    // LUT for 2^(i/256) in Q1.15 format
    inline constexpr uint16_t pow2_frac_lut_q1_15[256] PROGMEM = {
        32768, 32857, 32947, 33037, 33127, 33217, 33307, 33398, 33488, 33579, 33670, 33761, 33852, 33944, 34036, 34128,
        34220, 34313, 34406, 34499, 34592, 34686, 34780, 34874, 34968, 35063, 35158, 35253, 35349, 35445, 35541, 35637,
        35734, 35831, 35928, 36026, 36124, 36222, 36321, 36420, 36519, 36618, 36718, 36818, 36919, 37019, 37120, 37222,
        37323, 37425, 37527, 37630, 37732, 37835, 37939, 38042, 38146, 38251, 38355, 38460, 38565, 38671, 38777, 38883,
        38990, 39097, 39204, 39312, 39420, 39528, 39637, 39746, 39855, 39965, 40075, 40185, 40296, 40407, 40518, 40630,
        40742, 40854, 40967, 41080, 41194, 41308, 41422, 41537, 41652, 41767, 41883, 41999, 42115, 42232, 42349, 42467,
        42585, 42703, 42822, 42941, 43060, 43180, 43299, 43419, 43540, 43661, 43782, 43903, 44025, 44147, 44270, 44393,
        44516, 44640, 44764, 44888, 45013, 45138, 45263, 45389, 45515, 45642, 45769, 45896, 46024, 46152, 46280, 46409,
        46538, 46668, 46798, 46928, 47059, 47190, 47321, 47453, 47585, 47718, 47851, 47984, 48118, 48252, 48386, 48521,
        48656, 48792, 48928, 49064, 49201, 49338, 49476, 49614, 49752, 49891, 50030, 50170, 50310, 50450, 50591, 50732,
        50874, 51016, 51158, 51301, 51444, 51588, 51732, 51876, 52021, 52166, 52312, 52458, 52604, 52751, 52898, 53046,
        53194, 53342, 53491, 53640, 53790, 53940, 54090, 54241, 54392, 54544, 54696, 54848, 55001, 55154, 55308, 55462,
        55616, 55771, 55926, 56082, 56238, 56394, 56551, 56708, 56866, 57024, 57183, 57342, 57501, 57661, 57821, 57982,
        58143, 58304, 58466, 58628, 58791, 58954, 59117, 59281, 59445, 59610, 59775, 59940, 60106, 60272, 60439, 60606,
        60773, 60941, 61109, 61278, 61447, 61616, 61786, 61956, 62127, 62298, 62469, 62641, 62813, 62986, 63159, 63332,
        63506, 63680, 63855, 64030, 64205, 64381, 64557, 64734, 64911, 65088, 65266, 65360
    };

    /**
     * @brief Converts a Q8.8 fixed-point log2 value to a Q16.16 linear scale factor.
     * @param log2_val The log2 value in Q8.8 format.
     * @return The linear scale factor in Q16.16 format.
     */
    static int32_t log2_to_linear_q16(int16_t log2_val) {
        // The integer part of the log2 value determines the bit shift.
        // It must be in the range [-15, +15] to avoid overflow in the 32-bit result.
        int8_t int_part = constrain(log2_val >> 8, -15, 15);
        uint8_t frac_part = log2_val & 0xFF;

        // Calculate 2^int_part as Q16.16
        int32_t res = 1 << (16 + int_part);

        // Get 2^(frac_part/256) from LUT (Q1.15 format)
        uint16_t frac_factor_q1_15 = pgm_read_word_near(pow2_frac_lut_q1_15 + frac_part);

        // Multiply res (Q16.16) by the fractional factor (Q1.15)
        // The result is Q17.31, so we shift right by 15 to get Q16.16
        int64_t temp = static_cast<int64_t>(res) * frac_factor_q1_15;
        temp = (temp + (1 << 14)) >> 15; // with rounding

        if (temp > INT32_MAX) temp = INT32_MAX;
        return temp;
    }

    // LUT for log2(1 + i/256) in Q8 format
    inline constexpr uint8_t log2_frac_lut_q8[256] PROGMEM = {
        0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30,
        32, 34, 36, 38, 39, 41, 43, 45, 47, 49, 51, 52, 54, 56, 58, 60,
        61, 63, 65, 67, 68, 70, 72, 73, 75, 77, 78, 80, 82, 83, 85, 87,
        88, 90, 91, 93, 95, 96, 98, 99, 101, 102, 104, 105, 107, 108, 110, 111,
        113, 114, 116, 117, 119, 120, 122, 123, 125, 126, 128, 129, 131, 132, 133, 135,
        136, 138, 139, 140, 142, 143, 145, 146, 147, 149, 150, 151, 153, 154, 155, 157,
        158, 159, 161, 162, 163, 165, 166, 167, 168, 170, 171, 172, 174, 175, 176, 177,
        179, 180, 181, 182, 184, 185, 186, 187, 188, 190, 191, 192, 193, 194, 196, 197,
        198, 199, 200, 201, 203, 204, 205, 206, 207, 208, 209, 211, 212, 213, 214, 215,
        216, 217, 218, 219, 221, 222, 223, 224, 225, 226, 227, 228, 229, 230, 231, 232,
        233, 234, 235, 236, 237, 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248,
        249, 250, 251, 252, 253, 254, 255, 255
    };

    /**
     * @brief Calculates log2 of a fract16 value.
     * @param val A fract16 (uint16_t, 0-65535) representing a value in [0, 1).
     *            The value must be less than 1.0.
     * @return The log2 of the value, in Q8.8 fixed-point format.
     */
    static int16_t log2_f16(fract16 val) {
        static_assert(sizeof(int) == 4, "log2_f16 assumes 32-bit int for __builtin_clz");
        if (val == 0) return INT16_MIN; // log2(0) is -infinity
        if (val >= 65535 - 1) return 0; // log2(1) is 0

        // Count leading zeros to find the integer part of the log.
        uint8_t p = __builtin_clz(val) - 16;

        // Integer part of log2(val) for val in [0,1) is -(p+1)
        int8_t int_part = -(p + 1);

        // Normalize val to the range [1, 2) to calculate the fractional part.
        uint16_t z = val << (p + 1);

        // The fractional part of the log is log2(z/2^15), which is in [0,1).
        uint8_t frac_index = (z - 0x8000) >> 7;
        uint8_t frac_part = pgm_read_byte_near(log2_frac_lut_q8 + frac_index);
        return (int_part << 8) | frac_part;
    }

    /**
     * @brief Computes pow(base, exp).
     * @param base The base, a Q0.16 (fract16) value in the range [0, 1).
     * @param exp_q16_16 The exponent, a Q16.16 fixed-point value.
     * @return base^exp, as a Q16.16 value.
     */
    static int32_t pow_q16(fract16 base, int32_t exp_q16_16) {
        if (base == 0) return 0;
        if (base >= 65535 - 1 || exp_q16_16 == 0) {
            return 1 << 16;
        }

        // pow(b, e) = exp2(e * log2(b))
        int16_t log_base_q8_8 = log2_f16(base);

        // (Q16.16 * Q8.8) -> Q24.24
        int64_t product_q24_24 = (int64_t) exp_q16_16 * log_base_q8_8;

        // Convert to Q8.8 for exp2 function, clamping to a safe range.
        int16_t product_q8_8 = product_q24_24 >> 16;
        product_q8_8 = constrain(product_q8_8, -8 << 8, 8 << 8);

        // exp2(product) returns Q16.16
        return log2_to_linear_q16(product_q8_8);
    }
}
#endif //LED_SEGMENTS_SPECS_MATHUTILS_H
