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

#include "renderer/pipeline/maths/PatternMaths.h"

namespace PolarShader {
    PatternNormU0x16 patternNormalize(uint16_t value, uint16_t minValue, uint16_t maxValue) {
        if (value <= minValue) return PatternNormU0x16(0);
        if (value >= maxValue) return PatternNormU0x16(U0X16_MAX);

        uint16_t valueRange = static_cast<uint16_t>(maxValue - minValue);
        if (valueRange == 0) return PatternNormU0x16(0);

        return PatternNormU0x16(raw(toU0x16(static_cast<uint16_t>(value - minValue), valueRange)));
    }

    PatternNormU0x16 patternSmoothstepU16(uint16_t edge0, uint16_t edge1, uint16_t x) {
        if (edge0 >= edge1) {
            return x <= edge0 ? PatternNormU0x16(0) : PatternNormU0x16(U0X16_MAX);
        }
        if (x <= edge0) return PatternNormU0x16(0);
        if (x >= edge1) return PatternNormU0x16(U0X16_MAX);

        u0x16 progress = toU0x16(static_cast<uint16_t>(x - edge0), static_cast<uint16_t>(edge1 - edge0));

        uint32_t progressSquared = (static_cast<uint32_t>(raw(progress)) * raw(progress)) >> 16;
        uint32_t smoothstepFactor = U0X16_MAX * 3 - (2 * raw(progress));
        if (smoothstepFactor > U0X16_MAX * 3) smoothstepFactor = 0;

        uint32_t result = (progressSquared * smoothstepFactor) >> 16;
        if (result > U0X16_MAX) result = U0X16_MAX;

        return PatternNormU0x16(static_cast<uint16_t>(result));
    }
}
