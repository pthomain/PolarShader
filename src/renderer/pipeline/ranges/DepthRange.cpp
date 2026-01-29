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

#include "DepthRange.h"
#include "renderer/pipeline/maths/ScalarMaths.h"

namespace PolarShader {
    DepthRange::DepthRange(uint32_t min, uint32_t max)
        : min_depth(min), max_depth(max) {
    }

    MappedValue<uint32_t> DepthRange::map(SFracQ0_16 t) const {
        if (max_depth <= min_depth) return MappedValue(min_depth);

        uint32_t t_raw = clamp_frac_raw(raw(t));
        uint64_t span = static_cast<uint64_t>(max_depth) - static_cast<uint64_t>(min_depth);
        uint64_t scaled = (span * static_cast<uint64_t>(t_raw)) >> 16;
        uint64_t sum = static_cast<uint64_t>(min_depth) + scaled;

        if (sum > UINT32_MAX) sum = UINT32_MAX;
        return MappedValue(static_cast<uint32_t>(sum));
    }
}
