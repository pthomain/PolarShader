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

    fl::i32 scale32(fl::i32 value, FracQ0_16 scale) {
        uint16_t scale_raw = raw(scale);
        if (scale_raw == FRACT_Q0_16_MAX) return value;
        int64_t result = static_cast<int64_t>(value) * static_cast<int64_t>(scale_raw);
        result += (result >= 0) ? U16_HALF : -U16_HALF;
        result >>= 16;
        result = clamp_raw_q0_16_i64(result);
        return static_cast<int32_t>(result);
    }

    SFracQ0_16 mulSFracSat(SFracQ0_16 a, SFracQ0_16 b) {
        int64_t result = static_cast<int64_t>(raw(a)) * static_cast<int64_t>(raw(b));
        result += (result >= 0) ? U16_HALF : -U16_HALF;
        result >>= 16;
        result = clamp_raw_q0_16_i64(result);
        return SFracQ0_16(static_cast<int32_t>(result));
    }

    SFracQ0_16 mulSFracWrap(SFracQ0_16 a, SFracQ0_16 b) {
        int64_t result_i64 = static_cast<int64_t>(raw(a)) * static_cast<int64_t>(raw(b));
        result_i64 += (result_i64 >= 0) ? (1LL << 15) : -(1LL << 15);
        result_i64 >>= 16;
        return SFracQ0_16(static_cast<int32_t>(result_i64));
    }

    uint64_t sqrtU64(uint64_t value) {
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

    SFracQ0_16 scalarClampQ0_16Raw(int64_t raw_value) {
        return SFracQ0_16(static_cast<int32_t>(clamp_raw_q0_16_i64(raw_value)));
    }
}
