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

#ifdef ARDUINO
#include "FastLED.h"
#else
#include "native/FastLED.h"
#endif

namespace PolarShader {
    /** @brief The new unified sampling interface using normalized UV coordinates. */
    using UVLayer = fl::function<PatternNormU16(UV)>;

    using ColourLayer = fl::function<CRGB(FracQ0_16, FracQ0_16)>;
}

#endif //POLAR_SHADER_TRANSFORMS_BASE_LAYERS_H
