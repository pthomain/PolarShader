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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_SRC_HALF_LIFE_RANGE_H
#define POLAR_SHADER_PIPELINE_PATTERNS_SRC_HALF_LIFE_RANGE_H

// Shared half-life range used by FlowFieldPattern and TransportPattern.
// See ProfileRanges.h for the rationale on the anon-namespace-in-header
// pattern.

#include "renderer/pipeline/maths/units/Units.h"

namespace PolarShader {
    namespace {
        // Half-life range: 100ms to 5000ms, mapped from signal [0..1].
        constexpr uint16_t kHalfLifeMinMs = 100u;
        constexpr uint16_t kHalfLifeMaxMs = 5000u;
    }
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_SRC_HALF_LIFE_RANGE_H
