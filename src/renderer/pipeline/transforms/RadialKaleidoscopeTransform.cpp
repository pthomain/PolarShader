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

#include "RadialKaleidoscopeTransform.h"
#include "renderer/pipeline/maths/PolarMaths.h"

namespace PolarShader {
    struct RadialKaleidoscopeTransform::State {
        uint16_t divisions;
        bool mirrored;
    };

    RadialKaleidoscopeTransform::RadialKaleidoscopeTransform(uint16_t radialDivisions, bool isMirrored)
        : state(std::make_shared<State>(State{radialDivisions, isMirrored})) {
    }

    UVMap RadialKaleidoscopeTransform::operator()(const UVMap &layer) const {
        return [state = this->state, layer](UV uv) {
            UV polar_uv = cartesianToPolarUV(uv);
            
            uint32_t divisions = state->divisions;
            if (divisions > 1u) {
                uint32_t full_radius = static_cast<uint32_t>(FRACT_Q0_16_MAX) + 1u;
                uint32_t segment = full_radius / divisions;
                if (segment != 0u) {
                    uint32_t radius_raw = static_cast<uint32_t>(raw(polar_uv.v));
                    uint32_t index = radius_raw / segment;
                    if (index >= divisions) index = divisions - 1u;

                    uint32_t local = radius_raw - (index * segment);
                    if (state->mirrored && (index & 1u)) {
                        local = (segment - 1u) - local;
                    }

                    uint32_t scaled = (local * full_radius) / segment;
                    if (scaled >= full_radius) scaled = full_radius - 1u;
                    polar_uv.v = FracQ16_16(static_cast<int32_t>(scaled));
                }
            }

            return layer(polarToCartesianUV(polar_uv));
        };
    }
}
