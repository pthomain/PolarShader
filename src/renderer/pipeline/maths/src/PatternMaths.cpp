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
    PatternNormU16 patternNormalize(uint16_t value, uint16_t min_value, uint16_t max_value) {
        if (value <= min_value) return PatternNormU16(0);
        if (value >= max_value) return PatternNormU16(F16_MAX);

        uint16_t range = static_cast<uint16_t>(max_value - min_value);
        if (range == 0) return PatternNormU16(0);

        uint32_t scaled = static_cast<uint32_t>(value - min_value) * F16_MAX;
        return PatternNormU16(static_cast<uint16_t>(scaled / range));
    }

    PatternNormU16 patternSmoothstepU16(uint16_t edge0, uint16_t edge1, uint16_t x) {
        if (edge0 >= edge1) {
            return x <= edge0 ? PatternNormU16(0) : PatternNormU16(F16_MAX);
        }
        if (x <= edge0) return PatternNormU16(0);
        if (x >= edge1) return PatternNormU16(F16_MAX);

        uint32_t t_norm = (static_cast<uint32_t>(x - edge0) * F16_MAX) / (edge1 - edge0);
        uint16_t t = static_cast<uint16_t>(t_norm);

        uint32_t t_sq = (static_cast<uint32_t>(t) * t) >> 16;
        uint32_t three_minus_2t = F16_MAX * 3 - (2 * t);
        if (three_minus_2t > F16_MAX * 3) three_minus_2t = 0;

        uint32_t result = (t_sq * three_minus_2t) >> 16;
        if (result > F16_MAX) result = F16_MAX;

        return PatternNormU16(static_cast<uint16_t>(result));
    }
}
