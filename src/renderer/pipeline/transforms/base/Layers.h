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

#ifndef POLAR_SHADER_TRANSFORMS_BASE_LAYERS_H
#define POLAR_SHADER_TRANSFORMS_BASE_LAYERS_H

#include "FastLED.h"
#include "renderer/pipeline/units/CartesianUnits.h"
#include "renderer/pipeline/units/PatternUnits.h"
#include "renderer/pipeline/units/ScalarUnits.h"
#include "renderer/pipeline/units/UVUnits.h"

namespace PolarShader {
    using UVLayer = fl::function<PatternNormU16(UV)>;
    using PolarLayer = fl::function<PatternNormU16(FracQ0_16, FracQ0_16)>;
    // Cartesian coords are Q24.8 fixed-point representing Q0.16 lattice units with extra precision.
    using CartesianLayer = fl::function<PatternNormU16(CartQ24_8, CartQ24_8)>;
    using ColourLayer = fl::function<CRGB(FracQ0_16, FracQ0_16)>;
}

#endif //POLAR_SHADER_TRANSFORMS_BASE_LAYERS_H
