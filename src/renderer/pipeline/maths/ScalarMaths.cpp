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

#include "renderer/pipeline/maths/ScalarMaths.h"
#include <climits>
#include <cstring>

namespace PolarShader {
    namespace {
        template<typename T>
        constexpr T clamp_val(T v, T lo, T hi) {
            return (v < lo) ? lo : (v > hi ? hi : v);
        }

        inline int64_t clamp_raw_i64(int64_t value) {
            if (value > INT32_MAX) return INT32_MAX;
            if (value < INT32_MIN) return INT32_MIN;
            return value;
        }
    } // namespace

    fl::i32 scalarScaleByBounded(fl::i32 value, BoundedScalar scale) {
        uint16_t scale_raw = raw(scale);
        if (scale_raw == FRACT_Q0_16_MAX) return value;
        int64_t result = static_cast<int64_t>(value) * static_cast<int64_t>(scale_raw);
        result += (result >= 0) ? U16_HALF : -U16_HALF;
        result >>= 16;
        result = clamp_val(result, static_cast<int64_t>(INT32_MIN), static_cast<int64_t>(INT32_MAX));
        return static_cast<int32_t>(result);
    }

    UnboundedScalar scalarMulQ16_16Sat(UnboundedScalar a, UnboundedScalar b) {
        // Straight Q16.16 multiply with rounding and saturation.
        int64_t result = static_cast<int64_t>(a.asRaw()) * static_cast<int64_t>(b.asRaw());
        result += (result >= 0) ? U16_HALF : -U16_HALF;
        result >>= 16;
        result = clamp_val(result, static_cast<int64_t>(INT32_MIN), static_cast<int64_t>(INT32_MAX));
        return UnboundedScalar::fromRaw(static_cast<int32_t>(result));
    }

    UnboundedScalar scalarMulQ16_16Wrap(UnboundedScalar a, UnboundedScalar b) {
        int64_t result_i64 = (int64_t) a.asRaw() * (int64_t) b.asRaw();
        // Arithmetic shift right is implementation-defined for negatives; make it explicit by adding a bias
        // before shifting to mirror two's-complement behaviour.
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

    RawQ16_16 scalarAddWrapQ16_16(RawQ16_16 a, RawQ16_16 b) {
        uint32_t sum = static_cast<uint32_t>(raw(a)) + static_cast<uint32_t>(raw(b));
        int32_t signed_sum;
        memcpy(&signed_sum, &sum, sizeof(signed_sum));
        return RawQ16_16(signed_sum);
    }

    uint16_t scalarSqrtU32(uint32_t value) {
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

    uint64_t scalarSqrtU64(uint64_t value) {
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

    int64_t scalarScaleQ16_16ByTrig(RawQ16_16 magnitude, TrigQ1_15 trig_q1_15) {
        int64_t result = static_cast<int64_t>(raw(magnitude)) * static_cast<int64_t>(raw(trig_q1_15));
        result += (result >= 0) ? (1LL << 14) : -(1LL << 14);
        result >>= 15; // Q16.16 * Q1.15 => Q17.31 -> Q16.16
        return result;
    }

    UnboundedScalar scalarClampQ16_16Raw(int64_t raw_value) {
        return UnboundedScalar::fromRaw(static_cast<int32_t>(clamp_raw_i64(raw_value)));
    }

    UnboundedScalar unbound(BoundedScalar t, UnboundedScalar min_val, UnboundedScalar max_val) {
        int64_t min_raw = min_val.asRaw();
        int64_t max_raw = max_val.asRaw();
        int64_t span = max_raw - min_raw;
        if (span <= 0) return min_val;

        uint32_t t_raw = raw(t);
        int64_t scaled = (span * static_cast<int64_t>(t_raw)) >> 16;
        return scalarClampQ16_16Raw(min_raw + scaled);
    }

    BoundedScalar bound(UnboundedScalar value, UnboundedScalar min_val, UnboundedScalar max_val) {
        int64_t min_raw = min_val.asRaw();
        int64_t max_raw = max_val.asRaw();
        int64_t span = max_raw - min_raw;
        if (span <= 0) return BoundedScalar(0);

        int64_t val_raw = value.asRaw();
        if (val_raw < min_raw) val_raw = min_raw;
        if (val_raw > max_raw) val_raw = max_raw;

        uint64_t num = static_cast<uint64_t>(val_raw - min_raw) << 16;
        uint64_t t = num / static_cast<uint64_t>(span);
        if (t > 0xFFFFu) t = 0xFFFFu;
        return BoundedScalar(static_cast<uint16_t>(t));
    }
}
