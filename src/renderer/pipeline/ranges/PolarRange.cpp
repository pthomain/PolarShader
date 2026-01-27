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

#include "PolarRange.h"

namespace PolarShader {
    namespace {
        uint32_t wrap_frac_raw(int32_t raw_value) {
            return static_cast<uint32_t>(raw_value) & 0xFFFFu;
        }
    }

    PolarRange::PolarRange(FracQ0_16 min, FracQ0_16 max)
        : min_frac(min), max_frac(max) {
    }

    MappedSignal<FracQ0_16> PolarRange::map(SFracQ0_16 t) const {
        uint16_t min_raw = raw(min_frac);
        uint16_t max_raw = raw(max_frac);
        if (min_raw == max_raw) return MappedSignal(min_frac);
        uint32_t full_turn = static_cast<uint32_t>(FRACT_Q0_16_MAX) + 1u;

        uint32_t span = 0;
        if (max_raw > min_raw) {
            span = static_cast<uint32_t>(max_raw - min_raw);
        } else {
            span = (full_turn - static_cast<uint32_t>(min_raw)) + static_cast<uint32_t>(max_raw);
        }

        uint32_t t_raw = wrap_frac_raw(raw(t));
        uint32_t scaled = (span * t_raw) >> 16;
        uint32_t result = static_cast<uint32_t>(min_raw) + scaled;
        if (result >= full_turn) result -= full_turn;

        return MappedSignal(FracQ0_16(static_cast<uint16_t>(result)));
    }
}
