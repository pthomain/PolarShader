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

#include "MathUtils.h"
#include <cstring> // For memcpy
#ifdef ARDUINO
#include <Arduino.h>
#endif

namespace LEDSegments {

    namespace {
        template<typename T>
        constexpr T clamp_val(T v, T lo, T hi) {
            return (v < lo) ? lo : (v > hi ? hi : v);
        }

        // LUT for 2^(i/256) in Q1.15 format
        constexpr uint16_t pow2_frac_lut_q1_15[256] PROGMEM = {
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

        // LUT for log2(1 + i/256) in Q8 format
        constexpr uint8_t log2_frac_lut_q8[256] PROGMEM = {
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

        int32_t log2_to_linear_q16(int16_t log2_val) {
            int16_t int_part = clamp_val(static_cast<int16_t>(log2_val >> 8), (int16_t) -15, (int16_t) 15);
            uint8_t frac_part = log2_val & 0xFF;

            int32_t res = 1 << (16 + int_part);
            uint16_t frac_factor_q1_15 = pgm_read_word_near(pow2_frac_lut_q1_15 + frac_part);
            int64_t temp = static_cast<int64_t>(res) * frac_factor_q1_15;
            temp = (temp + (1 << 14)) >> 15;

            if (temp > INT32_MAX) temp = INT32_MAX;
            return static_cast<int32_t>(temp);
        }

        int16_t log2_f16(Units::FracQ0_16 val) {
            static_assert(sizeof(int) == 4, "log2_f16 assumes 32-bit int for __builtin_clz");
            if (val == 0) return INT16_MIN;
            if (val >= (Units::FRACT_Q0_16_MAX - 1)) return 0;

            uint8_t p = __builtin_clz(val) - 16;
            int8_t int_part = -(p + 1);
            uint16_t z = static_cast<uint16_t>(val << (p + 1));
            uint8_t frac_index = (z - Units::U16_HALF) >> 7;
            uint8_t frac_part = pgm_read_byte_near(log2_frac_lut_q8 + frac_index);
            return static_cast<int16_t>((int_part << 8) | frac_part);
        }

        int32_t pow_q16(Units::FracQ0_16 base, Units::RawFracQ16_16 exp_q16_16) {
            if (base == 0) return 0;
            if (base >= (Units::FRACT_Q0_16_MAX - 1) || exp_q16_16 == 0) {
                return static_cast<int32_t>(Units::Q16_16_ONE);
            }

            int16_t log_base_q8_8 = log2_f16(base);
            int64_t product_q24_24 = static_cast<int64_t>(exp_q16_16) * log_base_q8_8;
            int16_t product_q8_8 = static_cast<int16_t>(product_q24_24 >> 16);
            product_q8_8 = clamp_val(product_q8_8, (int16_t) (-8 << 8), (int16_t) (8 << 8));
            return log2_to_linear_q16(product_q8_8);
        }
    } // namespace

    fl::i16 scale_i16_by_f16(fl::i16 value, Units::FracQ0_16 scale) {
        if (scale == Units::FRACT_Q0_16_MAX) return value;
        int32_t result = static_cast<int32_t>(value) * static_cast<int32_t>(scale);
        result += (result >= 0) ? Units::U16_HALF : -Units::U16_HALF;
        result >>= 16;
        return static_cast<int16_t>(clamp_val(result, (int32_t) INT16_MIN, (int32_t) INT16_MAX));
    }

    fl::i32 scale_i32_by_f16(fl::i32 value, Units::FracQ0_16 scale) {
        if (scale == Units::FRACT_Q0_16_MAX) return value;
        int64_t result = static_cast<int64_t>(value) * static_cast<int64_t>(scale);
        result += (result >= 0) ? Units::U16_HALF : -Units::U16_HALF;
        result >>= 16;
        result = clamp_val(result, static_cast<int64_t>(INT32_MIN), static_cast<int64_t>(INT32_MAX));
        return static_cast<int32_t>(result);
    }

    fl::i32 scale_i32_by_q16_16(fl::i32 value, Units::RawFracQ16_16 scale) {
        if (scale == 0) return 0;
        if (scale == static_cast<Units::RawFracQ16_16>(Units::Q16_16_ONE)) return value;
        int64_t result = static_cast<int64_t>(value) * static_cast<int64_t>(scale);
        result += (result >= 0) ? Units::U16_HALF : -Units::U16_HALF;
        result >>= 16;
        return static_cast<int32_t>(clamp_val(result, (int64_t) INT32_MIN, (int64_t) INT32_MAX));
    }

    fl::i32 scale_q16_16_by_f16(fl::i32 value_q16_16, Units::FracQ0_16 scale_f16) {
        if (scale_f16 == Units::FRACT_Q0_16_MAX) return value_q16_16;
        int64_t result = static_cast<int64_t>(value_q16_16) * static_cast<int64_t>(scale_f16);
        result += (result >= 0) ? Units::U16_HALF : -Units::U16_HALF;
        result >>= 16;
        return static_cast<int32_t>(clamp_val(result, (int64_t) INT32_MIN, (int64_t) INT32_MAX));
    }

    Units::FracQ16_16 mul_q16_16_sat(Units::FracQ16_16 a, Units::FracQ16_16 b) {
        // Straight Q16.16 multiply with rounding and saturation.
        int64_t result = static_cast<int64_t>(a.asRaw()) * static_cast<int64_t>(b.asRaw());
        result += (result >= 0) ? Units::U16_HALF : -Units::U16_HALF;
        result >>= 16;
        result = clamp_val(result, static_cast<int64_t>(INT32_MIN), static_cast<int64_t>(INT32_MAX));
        return Units::FracQ16_16::fromRaw(static_cast<int32_t>(result));
    }

    Units::FracQ16_16 mul_q16_16_wrap(Units::FracQ16_16 a, Units::FracQ16_16 b) {
        int64_t result_i64 = (int64_t) a.asRaw() * (int64_t) b.asRaw();
        // Arithmetic shift right is implementation-defined for negatives; make it explicit by adding a bias
        // before shifting to mirror two's-complement behavior.
        result_i64 += (result_i64 >= 0) ? (1LL << 15) : -(1LL << 15);
        result_i64 >>= 16;

        // Use memcpy for a well-defined, portable bit-cast from uint32_t to int32_t,
        // preserving modulo-2^32 wrap. Interpretation of negative values assumes two's complement
        // (true on our targets).
        uint32_t result_u32 = static_cast<uint32_t>(result_i64);
        int32_t final_result;
        memcpy(&final_result, &result_u32, sizeof(final_result));

        return Units::FracQ16_16::fromRaw(final_result);
    }

    Units::RawFracQ16_16 add_sat_q16_16(Units::RawFracQ16_16 a, Units::RawFracQ16_16 b) {
        int64_t s = (int64_t) a + b;
        s = clamp_val(s, (int64_t) INT32_MIN, (int64_t) INT32_MAX);
        return (Units::RawFracQ16_16) s;
    }

    Units::RawFracQ16_16 add_wrap_q16_16(Units::RawFracQ16_16 a, Units::RawFracQ16_16 b) {
        uint32_t sum = static_cast<uint32_t>(a) + static_cast<uint32_t>(b);
        Units::RawFracQ16_16 result;
        memcpy(&result, &sum, sizeof(result));
        return result;
    }

    uint16_t sqrt_u32(uint32_t value) {
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

    Units::FracQ0_16 divide_u16_as_fract16(fl::u16 numerator, fl::u16 denominator) {
        if (denominator == 0) return UINT16_MAX;
        uint32_t temp = (static_cast<uint32_t>(numerator) << 16);
        temp = (temp + (denominator / 2)) / denominator;
        if (temp > UINT16_MAX) return UINT16_MAX;
        return static_cast<Units::FracQ0_16>(temp);
    }

    Units::AngleUnitsQ0_16 atan2_turns_approx(int16_t y, int16_t x) {
        if (x == 0 && y == 0) return 0;

        uint16_t abs_x = (x < 0) ? static_cast<uint16_t>(-x) : static_cast<uint16_t>(x);
        uint16_t abs_y = (y < 0) ? static_cast<uint16_t>(-y) : static_cast<uint16_t>(y);

        uint16_t max_val = (abs_x > abs_y) ? abs_x : abs_y;
        uint16_t min_val = (abs_x > abs_y) ? abs_y : abs_x;

        uint32_t z = (static_cast<uint32_t>(min_val) << 16) / max_val; // Q0.16
        uint32_t one_minus_z = Units::Q16_16_ONE - z;

        static constexpr uint32_t A_Q16 = Units::Q16_16_ONE / 8; // 0.125 turns in Q0.16
        static constexpr uint32_t B_Q16 = 2847u; // 0.04345 turns in Q0.16

        uint32_t inner = A_Q16 + ((B_Q16 * one_minus_z) >> 16);
        uint32_t base = (z * inner) >> 16; // 0..0.125 turns

        uint32_t angle = (abs_x >= abs_y) ? base : (Units::QUARTER_TURN_U16 - base);
        if (x < 0) {
            angle = Units::HALF_TURN_U16 - angle;
        }
        if (y < 0) {
            angle = Units::Q16_16_ONE - angle;
        }

        return static_cast<Units::AngleUnitsQ0_16>(angle & Units::ANGLE_U16_MAX);
    }

    // Computes base^exp with guards for retention-style use: exp is assumed non-negative; result is clamped to [0,1].
    Units::FracQ16_16 pow_f16_q16(Units::FracQ0_16 base, Units::FracQ16_16 exp) {
        Units::RawFracQ16_16 exp_raw = exp.asRaw();
        if (exp_raw <= 0 || base >= Units::FRACT_Q0_16_MAX) {
            return Units::FracQ16_16::fromRaw(Units::Q16_16_ONE);
        }
        if (base == 0) {
            return Units::FracQ16_16(0);
        }

        int32_t res_raw = pow_q16(base, exp_raw);
        return Units::FracQ16_16::fromRaw(res_raw);
    }

    uint64_t sqrt_u64(uint64_t value) {
        uint64_t op = value;
        uint64_t res = 0;
        uint64_t one = 1ULL << 62;

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
        return res;
    }

    int64_t scale_q16_16_by_trig(Units::RawFracQ16_16 magnitude, int32_t trig_q1_15) {
        int64_t product = static_cast<int64_t>(magnitude) * trig_q1_15;
        if (product >= 0) {
            product += (1LL << 14);
        } else {
            product -= (1LL << 14);
        }
        return product >> 15;
    }

    int16_t clamp_raw_to_i16(Units::RawFracQ16_16 raw) {
        int32_t value = raw >> 16;
        if (value > INT16_MAX) return INT16_MAX;
        if (value < INT16_MIN) return INT16_MIN;
        return static_cast<int16_t>(value);
    }

    int64_t clamp_raw(int64_t value) {
        if (value > INT32_MAX) return INT32_MAX;
        if (value < INT32_MIN) return INT32_MIN;
        return value;
    }
}
