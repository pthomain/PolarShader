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

#if defined(ARDUINO) || defined(__EMSCRIPTEN__)
#include "FastLED.h"
#else
#include "native/FastLED.h"
#endif
#include "renderer/pipeline/maths/units/Units.h"

namespace PolarShader {
    namespace detail {
        constexpr int64_t clampI64ToS0x16Sat(int64_t value) {
            if (value > S0X16_MAX) return S0X16_MAX;
            if (value < S0X16_MIN) return S0X16_MIN;
            return value;
        }
    }

    inline fl::i32 mulI32U0x16Sat(fl::i32 value, u0x16 scale) {
        uint16_t scale_raw = raw(scale);
        if (scale_raw == U0X16_MAX) return value;
        int64_t result = static_cast<int64_t>(value) * static_cast<int64_t>(scale_raw);
        result += (result >= 0) ? U16_HALF : -U16_HALF;
        result >>= 16;
        result = detail::clampI64ToS0x16Sat(result);
        return static_cast<int32_t>(result);
    }

    inline s0x16 mulS0x16Sat(s0x16 a, s0x16 b) {
        int64_t result = static_cast<int64_t>(raw(a)) * static_cast<int64_t>(raw(b));
        result += (result >= 0) ? U16_HALF : -U16_HALF;
        result >>= 16;
        result = detail::clampI64ToS0x16Sat(result);
        return s0x16(static_cast<int32_t>(result));
    }

    inline s0x16 scaleS0x16(s0x16 value, u0x16 scale) {
        int64_t result = static_cast<int64_t>(raw(value)) * static_cast<int64_t>(raw(scale));
        result += (result >= 0) ? U16_HALF : -U16_HALF;
        result >>= 16;
        result = detail::clampI64ToS0x16Sat(result);
        return s0x16(static_cast<int32_t>(result));
    }

    inline uint64_t sqrtU64Raw(uint64_t value) {
        uint64_t op = value;
        uint64_t res = 0;
        uint64_t one = 1ULL << 62;
        while (one > op) { one >>= 2; }
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

    inline s0x16 clampS0x16Sat(int64_t raw_value) {
        return s0x16(static_cast<int32_t>(detail::clampI64ToS0x16Sat(raw_value)));
    }

    // Map signed s0x16 [-1..1] raw values to unsigned u0x16 [0..1] with 0 at midpoint.
    // Clamps values outside [-1, 1].
    // -1 -> 0, 0 -> ~0.5, +1 -> 1.
    constexpr u0x16 toUnsignedClamped(s0x16 value) {
        int32_t raw_value = raw(value);
        if (raw_value <= S0X16_MIN) return u0x16(0);
        if (raw_value >= S0X16_MAX) return u0x16(U0X16_MAX);
        return u0x16(static_cast<uint16_t>(static_cast<uint32_t>(raw_value + S0X16_ONE) >> 1));
    }

    // Map signed s0x16 [-1..1] raw values to unsigned u0x16 [0..1] with 0 at midpoint.
    // Wraps values outside [-1, 1].
    // -1 -> 0, 0 -> ~0.5, +1 -> 1.
    constexpr u0x16 toUnsignedWrapped(s0x16 value) {
        // S0X16_ONE is 65536. We want (raw + 65536) / 2 mod 65536.
        // static_cast<uint32_t> handles wrapping correctly for our purposes.
        uint32_t shifted = static_cast<uint32_t>(raw(value) + S0X16_ONE);
        return u0x16(static_cast<uint16_t>(shifted >> 1));
    }

    // Map signed s0x16 [-1..1] raw values to unsigned u0x16 [0..1] with 0 at midpoint.
    // -1 -> 0, 0 -> ~0.5, +1 -> 1.
    constexpr u0x16 toUnsigned(s0x16 value) {
        return toUnsignedClamped(value);
    }

    // Map unsigned u0x16 [0..1] raw values to signed s0x16 [-1..1] with 0 at midpoint.
    // 0 -> -1, ~0.5 -> 0, +1 -> +1.
    constexpr s0x16 toSigned(u0x16 value) {
        uint32_t raw_value = raw(value);
        if (raw_value >= U0X16_MAX) return s0x16(S0X16_MAX);
        return s0x16(static_cast<int32_t>(raw_value << 1) - S0X16_ONE);
    }

    // Convert an integer ratio numerator/denominator to u0x16 [0..1], with saturation.
    constexpr u0x16 toU0x16(uint16_t numerator_raw, uint16_t denominator_raw) {
        if (denominator_raw == 0u) return u0x16(0);
        if (numerator_raw > denominator_raw) numerator_raw = denominator_raw;

        uint32_t scaled = static_cast<uint32_t>(numerator_raw) * U0X16_MAX;
        uint32_t rounded = (scaled + (denominator_raw >> 1)) / denominator_raw;
        if (rounded > U0X16_MAX) rounded = U0X16_MAX;
        return u0x16(static_cast<uint16_t>(rounded));
    }

    // Convert unsigned permille [0..1000] to unsigned fraction u0x16 [0..1], with saturation.
    constexpr u0x16 perMil(uint16_t value) {
        if (value > 1000u) value = 1000u;
        return toU0x16(value, 1000u);
    }
}

#endif // POLAR_SHADER_PIPELINE_MATHS_SCALARMATHS_H
