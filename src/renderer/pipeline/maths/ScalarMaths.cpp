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

namespace PolarShader {
    namespace {
        int64_t clamp_raw_q0_16_i64(int64_t value) {
            if (value > Q0_16_MAX) return Q0_16_MAX;
            if (value < Q0_16_MIN) return Q0_16_MIN;
            return value;
        }
    } // namespace

    fl::i32 scalarScaleByBounded(fl::i32 value, FracQ0_16 scale) {
        uint16_t scale_raw = raw(scale);
        if (scale_raw == FRACT_Q0_16_MAX) return value;
        int64_t result = static_cast<int64_t>(value) * static_cast<int64_t>(scale_raw);
        result += (result >= 0) ? U16_HALF : -U16_HALF;
        result >>= 16;
        result = clamp_raw_q0_16_i64(result);
        return static_cast<int32_t>(result);
    }

    ScalarQ0_16 scalarMulQ0_16Sat(ScalarQ0_16 a, ScalarQ0_16 b) {
        // Straight Q0.16 multiply with rounding and saturation.
        int64_t result = static_cast<int64_t>(raw(a)) * static_cast<int64_t>(raw(b));
        result += (result >= 0) ? U16_HALF : -U16_HALF;
        result >>= 16;
        result = clamp_raw_q0_16_i64(result);
        return ScalarQ0_16(static_cast<int32_t>(result));
    }

    ScalarQ0_16 scalarMulQ0_16Wrap(ScalarQ0_16 a, ScalarQ0_16 b) {
        int64_t result_i64 = static_cast<int64_t>(raw(a)) * static_cast<int64_t>(raw(b));
        result_i64 += (result_i64 >= 0) ? (1LL << 15) : -(1LL << 15);
        result_i64 >>= 16;
        return ScalarQ0_16(static_cast<int32_t>(result_i64));
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

    int32_t scalarScaleQ0_16ByTrig(ScalarQ0_16 magnitude, TrigQ0_16 trig_q0_16) {
        int64_t result = static_cast<int64_t>(raw(magnitude)) * static_cast<int64_t>(raw(trig_q0_16));
        result += (result >= 0) ? U16_HALF : -U16_HALF;
        result >>= 16; // Q0.16 * Q0.16 => Q0.16
        result = clamp_raw_q0_16_i64(result);
        return static_cast<int32_t>(result);
    }

    ScalarQ0_16 scalarClampQ0_16Raw(int64_t raw_value) {
        return ScalarQ0_16(static_cast<int32_t>(clamp_raw_q0_16_i64(raw_value)));
    }

    ScalarQ0_16 toScalar(FracQ0_16 t, ScalarQ0_16 min_val, ScalarQ0_16 max_val) {
        int64_t min_raw = raw(min_val);
        int64_t max_raw = raw(max_val);
        int64_t span = max_raw - min_raw;
        if (span <= 0) return min_val;

        uint32_t t_raw = raw(t);
        int64_t scaled = (span * static_cast<int64_t>(t_raw)) >> 16;
        return scalarClampQ0_16Raw(min_raw + scaled);
    }

    FracQ0_16 toFrac(ScalarQ0_16 value, ScalarQ0_16 min_val, ScalarQ0_16 max_val) {
        int64_t min_raw = raw(min_val);
        int64_t max_raw = raw(max_val);
        int64_t span = max_raw - min_raw;
        if (span <= 0) return FracQ0_16(0);

        int64_t val_raw = raw(value);
        if (val_raw < min_raw) val_raw = min_raw;
        if (val_raw > max_raw) val_raw = max_raw;

        uint64_t num = static_cast<uint64_t>(val_raw - min_raw) << 16;
        uint64_t t = num / static_cast<uint64_t>(span);
        if (t > 0xFFFFu) t = 0xFFFFu;
        return FracQ0_16(static_cast<uint16_t>(t));
    }
}
