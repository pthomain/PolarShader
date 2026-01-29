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

#include "PatternRange.h"
#include "renderer/pipeline/maths/ScalarMaths.h"

namespace PolarShader {
    PatternRange::PatternRange(uint16_t minValue, uint16_t maxValue)
        : min_value(minValue),
          max_value(maxValue) {
    }

    MappedValue<PatternNormU16> PatternRange::map(SFracQ0_16 t) const {
        uint32_t t_raw = clamp_frac_raw(raw(t));
        uint32_t span = static_cast<uint32_t>(max_value - min_value);
        if (span == 0) return MappedValue(PatternNormU16(min_value));

        uint32_t value = static_cast<uint32_t>(min_value) + ((span * t_raw) >> 16);
        return MappedValue(PatternNormU16(static_cast<uint16_t>(value)));
    }
}
