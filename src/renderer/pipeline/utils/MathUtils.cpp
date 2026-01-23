//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

/*
 * This file is part of PolarShader.
 *
 * PolarShader is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PolarShader is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PolarShader. If not, see <https://www.gnu.org/licenses/>.
 */

#include "MathUtils.h"
#ifdef ARDUINO
#include <Arduino.h>
#endif

namespace PolarShader {

    namespace {
        template<typename T>
        constexpr T clamp_val(T v, T lo, T hi) {
            return (v < lo) ? lo : (v > hi ? hi : v);
        }
    } // namespace

    fl::i32 scale_i32_by_f16(fl::i32 value, BoundedScalar scale) {
        uint16_t scale_raw = raw(scale);
        if (scale_raw == FRACT_Q0_16_MAX) return value;
        int64_t result = static_cast<int64_t>(value) * static_cast<int64_t>(scale_raw);
        result += (result >= 0) ? U16_HALF : -U16_HALF;
        result >>= 16;
        result = clamp_val(result, static_cast<int64_t>(INT32_MIN), static_cast<int64_t>(INT32_MAX));
        return static_cast<int32_t>(result);
    }

    fl::i32 scale_q16_16_by_f16(fl::i32 value_q16_16, BoundedScalar scale_f16) {
        uint16_t scale_raw = raw(scale_f16);
        if (scale_raw == FRACT_Q0_16_MAX) return value_q16_16;
        int64_t result = static_cast<int64_t>(value_q16_16) * static_cast<int64_t>(scale_raw);
        result += (result >= 0) ? U16_HALF : -U16_HALF;
        result >>= 16;
        return static_cast<int32_t>(clamp_val(result, (int64_t) INT32_MIN, (int64_t) INT32_MAX));
    }

    UnboundedScalar mul_q16_16_sat(UnboundedScalar a, UnboundedScalar b) {
        // Straight Q16.16 multiply with rounding and saturation.
        int64_t result = static_cast<int64_t>(a.asRaw()) * static_cast<int64_t>(b.asRaw());
        result += (result >= 0) ? U16_HALF : -U16_HALF;
        result >>= 16;
        result = clamp_val(result, static_cast<int64_t>(INT32_MIN), static_cast<int64_t>(INT32_MAX));
        return UnboundedScalar::fromRaw(static_cast<int32_t>(result));
    }

    UnboundedScalar mul_q16_16_wrap(UnboundedScalar a, UnboundedScalar b) {
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

        return UnboundedScalar::fromRaw(final_result);
    }

    RawQ16_16 add_wrap_q16_16(RawQ16_16 a, RawQ16_16 b) {
        uint32_t sum = static_cast<uint32_t>(raw(a)) + static_cast<uint32_t>(raw(b));
        int32_t signed_sum;
        memcpy(&signed_sum, &sum, sizeof(signed_sum));
        return RawQ16_16(signed_sum);
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

    BoundedAngle atan2_turns_approx(int16_t y, int16_t x) {
        if (x == 0 && y == 0) return BoundedAngle(0);

        uint16_t abs_x = (x < 0) ? static_cast<uint16_t>(-x) : static_cast<uint16_t>(x);
        uint16_t abs_y = (y < 0) ? static_cast<uint16_t>(-y) : static_cast<uint16_t>(y);

        uint16_t max_val = (abs_x > abs_y) ? abs_x : abs_y;
        uint16_t min_val = (abs_x > abs_y) ? abs_y : abs_x;

        uint32_t z = (static_cast<uint32_t>(min_val) << 16) / max_val; // Q0.16
        uint32_t one_minus_z = Q16_16_ONE - z;

        static constexpr uint32_t A_Q16 = Q16_16_ONE / 8; // 0.125 turns in Q0.16
        static constexpr uint32_t B_Q16 = 2847u; // 0.04345 turns in Q0.16

        uint32_t inner = A_Q16 + ((B_Q16 * one_minus_z) >> 16);
        uint32_t base = (z * inner) >> 16; // 0..0.125 turns

        uint32_t angle = (abs_x >= abs_y) ? base : (QUARTER_TURN_U16 - base);
        if (x < 0) {
            angle = HALF_TURN_U16 - angle;
        }
        if (y < 0) {
            angle = Q16_16_ONE - angle;
        }

        return BoundedAngle(static_cast<uint16_t>(angle & ANGLE_U16_MAX));
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

    int64_t scale_q16_16_by_trig(RawQ16_16 magnitude, TrigQ1_15 trig_q1_15) {
        int64_t product = static_cast<int64_t>(raw(magnitude)) * raw(trig_q1_15);
        if (product >= 0) {
            product += (1LL << 14);
        } else {
            product -= (1LL << 14);
        }
        return product >> 15;
    }

    UnboundedScalar clamp_q16_16_raw(int64_t raw_value) {
        return UnboundedScalar::fromRaw(static_cast<int32_t>(clamp_raw_i64(raw_value)));
    }

}
