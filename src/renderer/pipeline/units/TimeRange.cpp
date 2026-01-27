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

namespace PolarShader {
    namespace {
        uint32_t clamp_frac_raw(int32_t raw_value) {
            if (raw_value <= 0) return 0u;
            if (raw_value >= static_cast<int32_t>(FRACT_Q0_16_MAX)) return FRACT_Q0_16_MAX;
            return static_cast<uint32_t>(raw_value);
        }
    }

    TimeRange::TimeRange(TimeMillis durationMs)
        : duration_ms(durationMs) {
    }

    MappedSignal<TimeMillis> TimeRange::map(SFracQ0_16 t) const {
        uint64_t scaled = (static_cast<uint64_t>(duration_ms) * static_cast<uint64_t>(clamp_frac_raw(raw(t)))) >> 16;
        return MappedSignal(static_cast<TimeMillis>(scaled));
    }
}
