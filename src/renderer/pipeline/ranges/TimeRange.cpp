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

#include "TimeRange.h"
#include "renderer/pipeline/maths/ScalarMaths.h"

namespace PolarShader {
    TimeRange::TimeRange(TimeMillis durationMs)
        : duration_ms(durationMs) {
    }

    MappedValue<TimeMillis> TimeRange::map(SFracQ0_16 t) const {
        uint64_t scaled = (static_cast<uint64_t>(duration_ms) * static_cast<uint64_t>(clamp_frac_raw(raw(t)))) >> 16;
        return MappedValue(static_cast<TimeMillis>(scaled));
    }
}
