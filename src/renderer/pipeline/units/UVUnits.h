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

#ifndef POLAR_SHADER_PIPELINE_UNITS_UVUNITS_H
#define POLAR_SHADER_PIPELINE_UNITS_UVUNITS_H

#include "renderer/pipeline/units/ScalarUnits.h"

namespace PolarShader {
    /**
     * @brief Represents a normalized coordinate in UV space using FracQ16_16.
     * 
     * U and V are typically in the range [0.0, 1.0], mapped to [0, 65536].
     */
    struct UV {
        FracQ16_16 u;
        FracQ16_16 v;

        constexpr UV() : u(0), v(0) {}
        constexpr UV(FracQ16_16 u, FracQ16_16 v) : u(u), v(v) {}
    };
}

#endif // POLAR_SHADER_PIPELINE_UNITS_UVUNITS_H
