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

#include "ZoomRange.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include "renderer/pipeline/units/UnitConstants.h"
#include <utility>

namespace PolarShader {
    namespace {
        const int32_t MIN_SCALE = scale32(Q0_16_MAX, frac(160)); // 0.00625x
        const int32_t MAX_SCALE = Q0_16_ONE * 4; // 4x
    }

    ZoomRange::ZoomRange()
        : ZoomRange(SFracQ0_16(MIN_SCALE), SFracQ0_16(MAX_SCALE)) {
    }

    ZoomRange::ZoomRange(SFracQ0_16 minScale, SFracQ0_16 maxScale)
        : min_scale_raw(raw(minScale)),
          max_scale_raw(raw(maxScale)) {
        if (min_scale_raw > max_scale_raw) {
            std::swap(min_scale_raw, max_scale_raw);
        }
    }

    MappedValue<SFracQ0_16> ZoomRange::map(SFracQ0_16 t) const {
        int64_t span = static_cast<int64_t>(max_scale_raw) - static_cast<int64_t>(min_scale_raw);
        if (span == 0) return MappedValue(SFracQ0_16(min_scale_raw));

        uint32_t t_raw = clamp_frac_raw(raw(t));
        int64_t offset = (static_cast<int64_t>(t_raw) * span) >> 16;
        int64_t target = static_cast<int64_t>(min_scale_raw) + offset;

        if (target < min_scale_raw) target = min_scale_raw;
        if (target > max_scale_raw) target = max_scale_raw;
        return MappedValue(SFracQ0_16(static_cast<int32_t>(target)));
    }

}
