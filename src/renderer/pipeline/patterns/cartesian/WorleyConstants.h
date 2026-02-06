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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_CARTESIAN_WORLEYCONSTANTS_H
#define POLAR_SHADER_PIPELINE_PATTERNS_CARTESIAN_WORLEYCONSTANTS_H

#include "renderer/pipeline/units/UnitConstants.h"

namespace PolarShader {
    // Standard cell size for Cartesian lattice space (typically 0..256 wide).
    // A value of 10000 raw (approx 39.0 units) results in ~6.5 cells across the screen.
    inline constexpr int32_t WorleyCellUnit = 10000;

    enum class WorleyAliasing {
        None,
        Fast,
        Precise
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_CARTESIAN_WORLEYCONSTANTS_H
