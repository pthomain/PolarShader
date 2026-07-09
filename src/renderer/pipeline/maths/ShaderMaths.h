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

#ifndef POLAR_SHADER_PIPELINE_MATHS_SHADERMATHS_H
#define POLAR_SHADER_PIPELINE_MATHS_SHADERMATHS_H

#include "renderer/pipeline/maths/AngleMaths.h"
#include "renderer/pipeline/maths/FixedPointMaths.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include "renderer/pipeline/maths/units/Units.h"
#include <limits.h>

namespace PolarShader {
    struct Vec2Q16 {
        fl::s16x16 x;
        fl::s16x16 y;
    };

    inline fl::s16x16 dotQ16(Vec2Q16 a, Vec2Q16 b) {
        int64_t x =
            (static_cast<int64_t>(raw(a.x)) * static_cast<int64_t>(raw(b.x))) >> 16;
        int64_t y =
            (static_cast<int64_t>(raw(a.y)) * static_cast<int64_t>(raw(b.y))) >> 16;
        int64_t value = x + y;
        if (value > INT32_MAX) value = INT32_MAX;
        if (value < INT32_MIN) value = INT32_MIN;
        return fl::s16x16::from_raw(static_cast<int32_t>(value));
    }

    inline fl::s16x16 lengthQ16(Vec2Q16 v) {
        int64_t x = raw(v.x);
        int64_t y = raw(v.y);
        if (x < 0) x = -x;
        if (y < 0) y = -y;
        uint64_t sum =
            static_cast<uint64_t>(x) * static_cast<uint64_t>(x) +
            static_cast<uint64_t>(y) * static_cast<uint64_t>(y);
        uint64_t root = sqrtU64Raw(sum);
        if (root > static_cast<uint64_t>(INT32_MAX)) root = static_cast<uint64_t>(INT32_MAX);
        return fl::s16x16::from_raw(static_cast<int32_t>(root));
    }

    inline fl::s16x16 fractQ16(fl::s16x16 value) {
        return fl::s16x16::from_raw(static_cast<int32_t>(static_cast<uint32_t>(raw(value)) & F16_MAX));
    }

    inline Vec2Q16 fractQ16(Vec2Q16 value) {
        return Vec2Q16{fractQ16(value.x), fractQ16(value.y)};
    }

    inline fl::u16x16 divClampedQ16(fl::u16x16 num, fl::u16x16 den, fl::u16x16 maxValue) {
        if (raw(den) == 0u) return maxValue;
        uint64_t value = (static_cast<uint64_t>(raw(num)) << 16) / raw(den);
        if (value > raw(maxValue)) value = raw(maxValue);
        return fl::u16x16::from_raw(static_cast<uint32_t>(value));
    }

    namespace shader_detail {
        constexpr uint32_t EXP2_FRAC_FACTORS_Q16[16] = {
            92682u, 77936u, 71468u, 68438u,
            66971u, 66250u, 65892u, 65714u,
            65625u, 65580u, 65558u, 65547u,
            65542u, 65539u, 65537u, 65537u
        };
        constexpr int32_t LOG2_E_Q16 = 94548;

        inline int32_t log2Q16Raw(uint32_t xRaw) {
            if (xRaw == 0u) return INT32_MIN / 2;

            int32_t intPart = 0;
            uint32_t y = xRaw;
            while (y < 0x00010000u) {
                y <<= 1;
                --intPart;
            }
            while (y >= 0x00020000u) {
                y >>= 1;
                ++intPart;
            }

            uint32_t frac = 0;
            for (uint16_t bit = 0x8000u; bit != 0u; bit >>= 1) {
                uint64_t squared = (static_cast<uint64_t>(y) * static_cast<uint64_t>(y)) >> 16;
                y = static_cast<uint32_t>(squared);
                if (y >= 0x00020000u) {
                    y >>= 1;
                    frac |= bit;
                }
            }

            int64_t result = static_cast<int64_t>(intPart) * SF16_ONE + static_cast<int32_t>(frac);
            if (result > INT32_MAX) result = INT32_MAX;
            if (result < INT32_MIN) result = INT32_MIN;
            return static_cast<int32_t>(result);
        }

        inline uint32_t exp2Q16Raw(int32_t exponentRaw, uint32_t maxRaw) {
            int32_t intPart = exponentRaw / SF16_ONE;
            int32_t frac = exponentRaw % SF16_ONE;
            if (frac < 0) {
                frac += SF16_ONE;
                --intPart;
            }

            uint64_t result = SF16_ONE;
            uint16_t mask = 0x8000u;
            for (uint8_t i = 0; i < 16; ++i) {
                if (static_cast<uint32_t>(frac) & mask) {
                    result = (result * EXP2_FRAC_FACTORS_Q16[i]) >> 16;
                }
                mask >>= 1;
            }

            if (intPart >= 0) {
                if (intPart >= 16) return maxRaw;
                result <<= intPart;
                if (result > maxRaw) result = maxRaw;
            } else {
                int32_t shift = -intPart;
                if (shift >= 32) return 0u;
                result >>= shift;
            }

            return static_cast<uint32_t>(result);
        }
    }

    inline fl::u16x16 expNegQ16(fl::u16x16 x) {
        int64_t exponent = -((static_cast<int64_t>(raw(x)) * shader_detail::LOG2_E_Q16) >> 16);
        if (exponent < INT32_MIN) exponent = INT32_MIN;
        if (exponent > INT32_MAX) exponent = INT32_MAX;
        return fl::u16x16::from_raw(shader_detail::exp2Q16Raw(static_cast<int32_t>(exponent), SF16_ONE));
    }

    inline fl::u16x16 powQ16(fl::u16x16 x, uint16_t exponentPermille) {
        if (raw(x) == 0u) return fl::u16x16::from_raw(0);
        int64_t exponent =
            static_cast<int64_t>(shader_detail::log2Q16Raw(raw(x))) *
            static_cast<int64_t>(exponentPermille);
        exponent /= 1000;
        if (exponent < INT32_MIN) exponent = INT32_MIN;
        if (exponent > INT32_MAX) exponent = INT32_MAX;
        return fl::u16x16::from_raw(shader_detail::exp2Q16Raw(static_cast<int32_t>(exponent), UINT32_MAX));
    }

    inline RgbSample iqCosinePaletteQ16(fl::u16x16 t, PatternNormU16 value) {
        uint32_t turns = raw(t);
        PatternNormU16 r = PatternNormU16(raw(toUnsignedClamped(angleCosF16(f16(static_cast<uint16_t>(turns + 17236u))))));
        PatternNormU16 g = PatternNormU16(raw(toUnsignedClamped(angleCosF16(f16(static_cast<uint16_t>(turns + 27263u))))));
        PatternNormU16 b = PatternNormU16(raw(toUnsignedClamped(angleCosF16(f16(static_cast<uint16_t>(turns + 36504u))))));
        return RgbSample(r, g, b, value);
    }
}

#endif // POLAR_SHADER_PIPELINE_MATHS_SHADERMATHS_H
