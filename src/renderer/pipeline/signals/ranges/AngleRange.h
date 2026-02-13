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

#ifndef POLAR_SHADER_PIPELINE_RANGES_ANGLERANGE_H
#define POLAR_SHADER_PIPELINE_RANGES_ANGLERANGE_H

#include "renderer/pipeline/signals/ranges/Range.h"

namespace PolarShader {
    /**
     * @brief Maps signals to angular values (f16) with wrapping.
     *
     * Handles wrapping logic appropriate for angles (e.g., 0-360 degrees).
     */
    class AngleRange : public Range<f16> {
    public:
        AngleRange(f16 min = f16(0), f16 max = f16(F16_MAX))
            : min_frac(min), max_frac(max) {
        }

        f16 map(sf16 t) const override {
            uint16_t min_raw = raw(min_frac);
            uint16_t max_raw = raw(max_frac);
            if (min_raw == max_raw) return min_frac;
            uint32_t full_turn = static_cast<uint32_t>(F16_MAX) + 1u;

            uint32_t span = 0;
            if (max_raw > min_raw) {
                span = static_cast<uint32_t>(max_raw - min_raw);
            } else {
                span = (full_turn - static_cast<uint32_t>(min_raw)) + static_cast<uint32_t>(max_raw);
            }

            uint32_t t_raw = mapUnsigned(t);
            uint32_t scaled = (span * t_raw) >> 16;
            uint32_t result = static_cast<uint32_t>(min_raw) + scaled;
            if (result >= full_turn) result -= full_turn;

            return f16(static_cast<uint16_t>(result));
        }

    private:
        f16 min_frac = f16(0);
        f16 max_frac = f16(F16_MAX);
    };
}

#endif // POLAR_SHADER_PIPELINE_RANGES_ANGLERANGE_H
