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
#include <renderer/pipeline/utils/Units.h>

namespace PolarShader {
    using PolarLayer = fl::function<NoiseNormU16(AngleTurnsUQ16_16, RadiusQ0_16)>;
    using CartesianLayer = fl::function<NoiseNormU16(int32_t, int32_t)>;
    using NoiseLayer = fl::function<NoiseNormU16(uint32_t, uint32_t)>;
    using ColourLayer = fl::function<CRGB(AngleTurnsUQ16_16, RadiusQ0_16)>;
}

#endif //POLAR_SHADER_TRANSFORMS_BASE_LAYERS_H
