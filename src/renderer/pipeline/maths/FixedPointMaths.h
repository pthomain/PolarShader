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

#ifndef POLAR_SHADER_PIPELINE_MATHS_FIXEDPOINTMATHS_H
#define POLAR_SHADER_PIPELINE_MATHS_FIXEDPOINTMATHS_H

#include "renderer/pipeline/maths/units/Units.h"

namespace PolarShader {
    constexpr int32_t mulQ16Raw(int32_t aRaw, int32_t bRaw) {
        return static_cast<int32_t>((static_cast<int64_t>(aRaw) * bRaw) >> 16);
    }

    constexpr int32_t mulUnsignedQ16Raw(uint32_t value, uint32_t q16ScaleRaw) {
        return static_cast<int32_t>((static_cast<uint64_t>(value) * q16ScaleRaw) >> 16);
    }

    constexpr int32_t lerpQ16Raw(int32_t aRaw, int32_t bRaw, uint16_t tRaw) {
        return aRaw + static_cast<int32_t>((static_cast<int64_t>(bRaw - aRaw) * tRaw) >> 16);
    }

    constexpr uint16_t lerpU16ByQ16(uint16_t a, uint16_t b, f16 t) {
        uint32_t tRaw = raw(t);
        uint32_t oneMinusT = 65536u - tRaw;
        uint32_t value = (
            static_cast<uint32_t>(a) * oneMinusT +
            static_cast<uint32_t>(b) * tRaw +
            32768u
        ) >> 16;
        return static_cast<uint16_t>(value);
    }

    constexpr int32_t clampI32(int32_t value, int32_t minValue, int32_t maxValue) {
        return value < minValue ? minValue : (value > maxValue ? maxValue : value);
    }

    constexpr uint16_t clampU16(uint32_t value) {
        return static_cast<uint16_t>(value > 65535u ? 65535u : value);
    }

    inline fl::s16x16 mulS16x16(fl::s16x16 value, sf16 scale) {
        return fl::s16x16::from_raw(mulQ16Raw(raw(value), raw(scale)));
    }

    inline fl::s16x16 mulS16x16(sf16 scale, fl::s16x16 value) {
        return mulS16x16(value, scale);
    }

    inline fl::s16x16 lerpS16x16(fl::s16x16 a, fl::s16x16 b, f16 t) {
        return fl::s16x16::from_raw(lerpQ16Raw(raw(a), raw(b), raw(t)));
    }

    inline fl::s16x16 clampS16x16(fl::s16x16 value, fl::s16x16 minValue, fl::s16x16 maxValue) {
        return fl::s16x16::from_raw(clampI32(raw(value), raw(minValue), raw(maxValue)));
    }

    constexpr uint16_t scaleU16ByF16(uint16_t value, f16 scale) {
        return static_cast<uint16_t>(mulUnsignedQ16Raw(value, raw(scale)));
    }
}

#endif // POLAR_SHADER_PIPELINE_MATHS_FIXEDPOINTMATHS_H
