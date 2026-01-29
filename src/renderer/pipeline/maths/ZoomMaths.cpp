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

#include "ZoomMaths.h"

namespace PolarShader {
    int32_t zoomMinScaleRaw(const ZoomRange &range) {
        return raw(range.map(SFracQ0_16(0)).get());
    }

    int32_t zoomMaxScaleRaw(const ZoomRange &range) {
        return raw(range.map(SFracQ0_16(Q0_16_ONE)).get());
    }

    SFracQ0_16 zoomNormalize(SFracQ0_16 value, int32_t min_scale_raw, int32_t max_scale_raw) {
        int64_t span = static_cast<int64_t>(max_scale_raw) - static_cast<int64_t>(min_scale_raw);
        if (span <= 0) return SFracQ0_16(Q0_16_ONE);

        int64_t numer = static_cast<int64_t>(raw(value)) - static_cast<int64_t>(min_scale_raw);
        if (numer < 0) numer = 0;
        if (numer > span) numer = span;
        int64_t t_raw = (numer << 16) / span;
        if (t_raw < 0) t_raw = 0;
        if (t_raw > Q0_16_ONE) t_raw = Q0_16_ONE;
        return SFracQ0_16(static_cast<int32_t>(t_raw));
    }
}
