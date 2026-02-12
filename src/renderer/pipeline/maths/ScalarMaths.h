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
        constexpr int64_t clampI64ToSf16Sat(int64_t value) {
            if (value > SF16_MAX) return SF16_MAX;
            if (value < SF16_MIN) return SF16_MIN;
            return value;
        }

        constexpr uint64_t clampU64ToF16Sat(uint64_t value) {
            if (value > F16_MAX) return F16_MAX;
            return value;
        }
    }

    inline fl::i32 mulI32F16Sat(fl::i32 value, f16 scale) {
        uint16_t scale_raw = raw(scale);
        if (scale_raw == F16_MAX) return value;
        int64_t result = static_cast<int64_t>(value) * static_cast<int64_t>(scale_raw);
        result += (result >= 0) ? U16_HALF : -U16_HALF;
        result >>= 16;
        result = detail::clampI64ToSf16Sat(result);
        return static_cast<int32_t>(result);
    }

    inline sf16 mulSf16Sat(sf16 a, sf16 b) {
        int64_t result = static_cast<int64_t>(raw(a)) * static_cast<int64_t>(raw(b));
        result += (result >= 0) ? U16_HALF : -U16_HALF;
        result >>= 16;
        result = detail::clampI64ToSf16Sat(result);
        return sf16(static_cast<int32_t>(result));
    }

    inline sf16 mulSf16Wrap(sf16 a, sf16 b) {
        int64_t result_i64 = static_cast<int64_t>(raw(a)) * static_cast<int64_t>(raw(b));
        result_i64 += (result_i64 >= 0) ? (1LL << 15) : -(1LL << 15);
        result_i64 >>= 16;
        return sf16(static_cast<int32_t>(result_i64));
    }

    inline sf16 divSf16Sat(sf16 numerator, sf16 denominator) {
        int32_t denominator_raw = raw(denominator);
        if (denominator_raw == 0) return sf16(0);

        int64_t scaled_numerator = static_cast<int64_t>(raw(numerator)) * SF16_ONE;
        int64_t abs_numerator = (scaled_numerator < 0) ? -scaled_numerator : scaled_numerator;
        int64_t abs_denominator = (denominator_raw < 0) ? -static_cast<int64_t>(denominator_raw) : denominator_raw;
        int64_t quotient = (abs_numerator + (abs_denominator >> 1)) / abs_denominator;

        bool negative = (scaled_numerator < 0) != (denominator_raw < 0);
        int64_t signed_quotient = negative ? -quotient : quotient;
        signed_quotient = detail::clampI64ToSf16Sat(signed_quotient);
        return sf16(static_cast<int32_t>(signed_quotient));
    }

    inline sf16 divSf16Wrap(sf16 numerator, sf16 denominator) {
        int32_t denominator_raw = raw(denominator);
        if (denominator_raw == 0) return sf16(0);

        int64_t scaled_numerator = static_cast<int64_t>(raw(numerator)) * SF16_ONE;
        int64_t abs_numerator = (scaled_numerator < 0) ? -scaled_numerator : scaled_numerator;
        int64_t abs_denominator = (denominator_raw < 0) ? -static_cast<int64_t>(denominator_raw) : denominator_raw;
        int64_t quotient = (abs_numerator + (abs_denominator >> 1)) / abs_denominator;

        bool negative = (scaled_numerator < 0) != (denominator_raw < 0);
        int64_t signed_quotient = negative ? -quotient : quotient;
        return sf16(static_cast<int32_t>(signed_quotient));
    }

    inline f16 mulF16Sat(f16 a, f16 b) {
        uint64_t result = static_cast<uint64_t>(raw(a)) * static_cast<uint64_t>(raw(b));
        result += U16_HALF;
        result >>= 16;
        result = detail::clampU64ToF16Sat(result);
        return f16(static_cast<uint16_t>(result));
    }

    inline f16 mulF16Wrap(f16 a, f16 b) {
        uint64_t result = static_cast<uint64_t>(raw(a)) * static_cast<uint64_t>(raw(b));
        result += U16_HALF;
        result >>= 16;
        return f16(static_cast<uint16_t>(result));
    }

    inline f16 divF16Sat(f16 numerator, f16 denominator) {
        uint16_t denominator_raw = raw(denominator);
        if (denominator_raw == 0) return f16(0);

        uint64_t scaled_numerator = static_cast<uint64_t>(raw(numerator)) << 16;
        uint64_t quotient = (scaled_numerator + (denominator_raw >> 1)) / denominator_raw;
        quotient = detail::clampU64ToF16Sat(quotient);
        return f16(static_cast<uint16_t>(quotient));
    }

    inline f16 divF16Wrap(f16 numerator, f16 denominator) {
        uint16_t denominator_raw = raw(denominator);
        if (denominator_raw == 0) return f16(0);

        uint64_t scaled_numerator = static_cast<uint64_t>(raw(numerator)) << 16;
        uint64_t quotient = (scaled_numerator + (denominator_raw >> 1)) / denominator_raw;
        return f16(static_cast<uint16_t>(quotient));
    }

    inline uint64_t sqrtU64Raw(uint64_t value) {
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

    inline sf16 clampSf16Sat(int64_t raw_value) {
        return sf16(static_cast<int32_t>(detail::clampI64ToSf16Sat(raw_value)));
    }

    // Map signed sf16 [-1..1] raw values to unsigned f16 [0..1] with 0 at midpoint.
    // -1 -> 0, 0 -> ~0.5, +1 -> 1.
    constexpr f16 toUnsigned(sf16 value) {
        int32_t raw_value = raw(value);
        if (raw_value <= SF16_MIN) return f16(0);
        if (raw_value >= SF16_MAX) return f16(F16_MAX);
        return f16(static_cast<uint16_t>(static_cast<uint32_t>(raw_value + SF16_ONE) >> 1));
    }

    // Map unsigned f16 [0..1] raw values to signed sf16 [-1..1] with 0 at midpoint.
    // 0 -> -1, ~0.5 -> 0, +1 -> +1.
    constexpr sf16 toSigned(f16 value) {
        uint32_t raw_value = raw(value);
        if (raw_value >= F16_MAX) return sf16(SF16_MAX);
        return sf16(static_cast<int32_t>(raw_value << 1) - SF16_ONE);
    }

    // Convert signed permille [-1000..1000] to signed scalar sf16 [-1..1], with saturation.
    constexpr sf16 sPerMil(int16_t value) {
        if (value > 1000) value = 1000;
        if (value < -1000) value = -1000;

        int32_t raw_value = value * SF16_ONE / 1000;
        return sf16(raw_value);
    }

    // Convert an integer ratio numerator/denominator to f16 [0..1], with saturation.
    constexpr f16 toF16(uint16_t numerator_raw, uint16_t denominator_raw) {
        if (denominator_raw == 0u) return f16(0);
        if (numerator_raw > denominator_raw) numerator_raw = denominator_raw;

        uint32_t scaled = static_cast<uint32_t>(numerator_raw) * F16_MAX;
        uint32_t rounded = (scaled + (denominator_raw >> 1)) / denominator_raw;
        if (rounded > F16_MAX) rounded = F16_MAX;
        return f16(static_cast<uint16_t>(rounded));
    }

    // Convert unsigned permille [0..1000] to unsigned fraction f16 [0..1], with saturation.
    constexpr f16 perMil(uint16_t value) {
        if (value > 1000u) value = 1000u;
        return toF16(value, 1000u);
    }
}

#endif // POLAR_SHADER_PIPELINE_MATHS_SCALARMATHS_H
