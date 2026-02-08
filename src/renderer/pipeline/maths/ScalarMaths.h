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

#ifndef POLAR_SHADER_PIPELINE_MATHS_SCALARMATHS_H
#define POLAR_SHADER_PIPELINE_MATHS_SCALARMATHS_H

#ifdef ARDUINO
#include "FastLED.h"
#else
#include "native/FastLED.h"
#endif
#include "renderer/pipeline/maths/units/Units.h"

namespace PolarShader {
    namespace detail {
        constexpr int64_t clamp_raw_q0_16_i64(int64_t value) {
            if (value > Q0_16_MAX) return Q0_16_MAX;
            if (value < Q0_16_MIN) return Q0_16_MIN;
            return value;
        }
    }

    inline fl::i32 scale32(fl::i32 value, FracQ0_16 scale) {
        uint16_t scale_raw = raw(scale);
        if (scale_raw == FRACT_Q0_16_MAX) return value;
        int64_t result = static_cast<int64_t>(value) * static_cast<int64_t>(scale_raw);
        result += (result >= 0) ? U16_HALF : -U16_HALF;
        result >>= 16;
        result = detail::clamp_raw_q0_16_i64(result);
        return static_cast<int32_t>(result);
    }

    inline SFracQ0_16 mulSFracSat(SFracQ0_16 a, SFracQ0_16 b) {
        int64_t result = static_cast<int64_t>(raw(a)) * static_cast<int64_t>(raw(b));
        result += (result >= 0) ? U16_HALF : -U16_HALF;
        result >>= 16;
        result = detail::clamp_raw_q0_16_i64(result);
        return SFracQ0_16(static_cast<int32_t>(result));
    }

    inline SFracQ0_16 mulSFracWrap(SFracQ0_16 a, SFracQ0_16 b) {
        int64_t result_i64 = static_cast<int64_t>(raw(a)) * static_cast<int64_t>(raw(b));
        result_i64 += (result_i64 >= 0) ? (1LL << 15) : -(1LL << 15);
        result_i64 >>= 16;
        return SFracQ0_16(static_cast<int32_t>(result_i64));
    }

    inline uint64_t sqrtU64(uint64_t value) {
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

    inline SFracQ0_16 scalarClampQ0_16Raw(int64_t raw_value) {
        return SFracQ0_16(static_cast<int32_t>(detail::clamp_raw_q0_16_i64(raw_value)));
    }

    /**
     * @brief Create a Q0.16 bounded scalar from a rational fraction without floating point.
     */
    constexpr FracQ0_16 frac(uint16_t numerator, uint16_t denominator) {
        if (numerator == 0 || denominator == 0) return FracQ0_16(0);
        uint64_t raw_value = (static_cast<uint64_t>(FRACT_Q0_16_MAX) * numerator) / denominator;
        if (raw_value > FRACT_Q0_16_MAX) raw_value = FRACT_Q0_16_MAX;
        return FracQ0_16(static_cast<uint16_t>(raw_value));
    }

    constexpr FracQ0_16 frac(uint16_t denominator) {
        return frac(1u, denominator);
    }

    /**
     * @brief Create a signed Q0.16 scalar from a rational fraction without floating point.
     *
     * Returns the negated result of frac().
     */
    constexpr SFracQ0_16 sFrac(uint16_t numerator, uint16_t denominator) {
        FracQ0_16 value = frac(numerator, denominator);
        return SFracQ0_16(-static_cast<int32_t>(raw(value)));
    }

    constexpr SFracQ0_16 sFrac(uint16_t denominator) {
        return sFrac(1u, denominator);
    }

    constexpr FracQ0_16 perMil(uint16_t perMil) {
        if (perMil == 0) return FracQ0_16(0);
        uint32_t raw_value = static_cast<uint32_t>((static_cast<uint64_t>(FRACT_Q0_16_MAX) * perMil) / 1000);
        if (raw_value > FRACT_Q0_16_MAX) raw_value = FRACT_Q0_16_MAX;
        return FracQ0_16(static_cast<uint16_t>(raw_value));
    }

    /**
     * @brief Create a signed Q0.16 scalar from a per-mil fraction.
     *
     * Returns the negated result of perMil().
     */
    constexpr SFracQ0_16 sPerMil(uint16_t perMilValue) {
        FracQ0_16 value = perMil(perMilValue);
        return SFracQ0_16(-static_cast<int32_t>(raw(value)));
    }

    // Map signed Q0.16 [-1..1] raw values to unsigned Q0.16 [0..1] with 0 at midpoint.
    // -1 -> 0, 0 -> ~0.5, +1 -> 1.
    constexpr uint32_t signed_to_unit_raw(int32_t raw_value) {
        if (raw_value <= Q0_16_MIN) return 0u;
        if (raw_value >= Q0_16_MAX) return FRACT_Q0_16_MAX;
        return static_cast<uint32_t>(raw_value + Q0_16_ONE) >> 1;
    }
}

#endif // POLAR_SHADER_PIPELINE_MATHS_SCALARMATHS_H
