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

#ifndef POLAR_SHADER_PIPELINE_RANGES_PATTERN_RANGE_H
#define POLAR_SHADER_PIPELINE_RANGES_PATTERN_RANGE_H

#include <cstdint>
#include "renderer/pipeline/units/PatternUnits.h"
#include "renderer/pipeline/units/UnitConstants.h"

namespace PolarShader {
    /**
     * @brief Maps arbitrary uint16_t inputs into the normalized PatternNormU16 domain.
     */
    class PatternRange {
    public:
        PatternRange(uint16_t minValue = 0, uint16_t maxValue = FRACT_Q0_16_MAX);

        uint16_t min() const { return min_value; }
        uint16_t max() const { return max_value; }

        PatternNormU16 normalize(uint16_t value) const;

        static PatternNormU16 smoothstep_u16(uint16_t edge0, uint16_t edge1, uint16_t x);

    private:
        uint16_t min_value;
        uint16_t max_value;
    };
}

#endif // POLAR_SHADER_PIPELINE_RANGES_PATTERN_RANGE_H
