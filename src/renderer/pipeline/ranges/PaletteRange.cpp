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

#include "PaletteRange.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include <utility>

namespace PolarShader {
    PaletteRange::PaletteRange(uint8_t minValue, uint8_t maxValue)
        : min_value(minValue),
          max_value(maxValue) {
        if (min_value > max_value) {
            std::swap(min_value, max_value);
        }
    }

    MappedValue<uint8_t> PaletteRange::map(SFracQ0_16 t) const {
        uint32_t t_raw = clamp_frac_raw(raw(t));
        uint32_t span = static_cast<uint32_t>(max_value - min_value);
        if (span == 0) return MappedValue(min_value);

        uint32_t value = static_cast<uint32_t>(min_value) + ((span * t_raw) >> 16);
        return MappedValue(static_cast<uint8_t>(value));
    }
}
