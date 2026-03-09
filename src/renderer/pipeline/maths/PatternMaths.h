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

#ifndef POLAR_SHADER_PIPELINE_MATHS_PATTERNMATHS_H
#define POLAR_SHADER_PIPELINE_MATHS_PATTERNMATHS_H

#include "renderer/pipeline/maths/FixedPointMaths.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include "renderer/pipeline/maths/units/Units.h"

namespace PolarShader {
    PatternNormU16 patternNormalize(uint16_t value, uint16_t minValue, uint16_t maxValue);

    PatternNormU16 patternSmoothstepU16(uint16_t edge0, uint16_t edge1, uint16_t x);

    namespace detail {
        inline PatternNormU16 sampleScalarGrid(
            const uint16_t *gridValues,
            uint16_t width,
            uint16_t height,
            fl::u16x16 sampleX,
            fl::u16x16 sampleY,
            bool wrapAtEdges
        ) {
            uint16_t leftX = static_cast<uint16_t>(raw(sampleX) >> 16);
            uint16_t topY = static_cast<uint16_t>(raw(sampleY) >> 16);
            uint16_t rightX = static_cast<uint16_t>(
                (leftX + 1u < width) ? (leftX + 1u) : (wrapAtEdges ? 0u : (width - 1u))
            );
            uint16_t bottomY = static_cast<uint16_t>(
                (topY + 1u < height) ? (topY + 1u) : (wrapAtEdges ? 0u : (height - 1u))
            );
            f16 xMix(static_cast<uint16_t>(raw(sampleX) & F16_MAX));
            f16 yMix(static_cast<uint16_t>(raw(sampleY) & F16_MAX));

            uint16_t topLeft = gridValues[static_cast<size_t>(topY) * width + leftX];
            uint16_t topRight = gridValues[static_cast<size_t>(topY) * width + rightX];
            uint16_t bottomLeft = gridValues[static_cast<size_t>(bottomY) * width + leftX];
            uint16_t bottomRight = gridValues[static_cast<size_t>(bottomY) * width + rightX];

            uint16_t topRow = lerpU16ByQ16(topLeft, topRight, xMix);
            uint16_t bottomRow = lerpU16ByQ16(bottomLeft, bottomRight, xMix);
            return PatternNormU16(lerpU16ByQ16(topRow, bottomRow, yMix));
        }
    }

    inline PatternNormU16 sampleScalarGridClamped(const uint16_t *gridValues, uint16_t width, uint16_t height, UV uv) {
        if (!gridValues || width == 0 || height == 0) return PatternNormU16(0);

        fl::s16x16 clampedU = clampS16x16(uv.u, fl::s16x16::from_raw(0), fl::s16x16::from_raw(F16_MAX));
        fl::s16x16 clampedV = clampS16x16(uv.v, fl::s16x16::from_raw(0), fl::s16x16::from_raw(F16_MAX));
        fl::u16x16 sampleX = fl::u16x16::from_raw(static_cast<uint32_t>(raw(clampedU)) * width);
        fl::u16x16 sampleY = fl::u16x16::from_raw(static_cast<uint32_t>(raw(clampedV)) * height);

        return detail::sampleScalarGrid(gridValues, width, height, sampleX, sampleY, false);
    }

    inline PatternNormU16 sampleScalarGridWrapped(const uint16_t *gridValues, uint16_t width, uint16_t height, UV uv) {
        if (!gridValues || width == 0 || height == 0) return PatternNormU16(0);

        int32_t wrappedURaw = raw(uv.u) % ANGLE_FULL_TURN_U32;
        if (wrappedURaw < 0) wrappedURaw += ANGLE_FULL_TURN_U32;
        int32_t wrappedVRaw = raw(uv.v) % ANGLE_FULL_TURN_U32;
        if (wrappedVRaw < 0) wrappedVRaw += ANGLE_FULL_TURN_U32;
        fl::u16x16 sampleX = fl::u16x16::from_raw(static_cast<uint32_t>(wrappedURaw) * width);
        fl::u16x16 sampleY = fl::u16x16::from_raw(static_cast<uint32_t>(wrappedVRaw) * height);

        return detail::sampleScalarGrid(gridValues, width, height, sampleX, sampleY, true);
    }
}

#endif // POLAR_SHADER_PIPELINE_MATHS_PATTERNMATHS_H
