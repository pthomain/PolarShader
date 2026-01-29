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

#include "SFracRange.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include <utility>

namespace PolarShader {
    SFracRange::SFracRange(SFracQ0_16 minValue, SFracQ0_16 maxValue)
        : min_raw(raw(minValue)),
          max_raw(raw(maxValue)) {
        if (min_raw > max_raw) {
            std::swap(min_raw, max_raw);
        }
    }

    MappedValue<SFracQ0_16> SFracRange::map(SFracQ0_16 t) const {
        int64_t span = static_cast<int64_t>(max_raw) - static_cast<int64_t>(min_raw);
        if (span == 0) return MappedValue(SFracQ0_16(min_raw));

        uint32_t t_raw = clamp_frac_raw(raw(t));
        int64_t scaled = (span * static_cast<int64_t>(t_raw)) >> 16;
        return MappedValue(SFracQ0_16(static_cast<int32_t>(static_cast<int64_t>(min_raw) + scaled)));
    }
}
