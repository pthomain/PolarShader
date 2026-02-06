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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_H
#define POLAR_SHADER_PIPELINE_PATTERNS_H

#include <memory>
#include <renderer/pipeline/patterns/cartesian/WorleyConstants.h>
#include <renderer/pipeline/patterns/cartesian/WorleyPatterns.h>

namespace PolarShader {
    // Pattern factory helpers. These mirror the Presets API for quick creation.

    // Nearest-distance Worley field (F1).
    std::unique_ptr<UVPattern> worleyPattern(
        CartQ24_8 cellSize = CartQ24_8(WorleyCellUnit),
        WorleyAliasing aliasingMode = WorleyAliasing::Fast
    );

    // Hash-based Voronoi cell ID mapped to 0..65535.
    std::unique_ptr<UVPattern> voronoiPattern(
        CartQ24_8 cellSize = CartQ24_8(WorleyCellUnit),
        WorleyAliasing aliasingMode = WorleyAliasing::Fast
    );

    // Base noise pattern (inoise16) with signed cartesian input.
    std::unique_ptr<UVPattern> noisePattern();

    // fBm noise pattern with configurable octaves (default 4).
    std::unique_ptr<UVPattern> fbmNoisePattern(
        fl::u8 octaves = 4
    );

    // Turbulence noise (absolute-value style).
    std::unique_ptr<UVPattern> turbulenceNoisePattern();

    // Ridged noise (inverted turbulence).
    std::unique_ptr<UVPattern> ridgedNoisePattern();

    // Hexagon tiling with N-colouring (no adjacent matches when colorCount >= 3).
    std::unique_ptr<UVPattern> hexTilingPattern(
        uint16_t hexRadius = 10000,
        uint8_t colorCount = 4,
        uint16_t edgeSoftness = 0
    );
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_H
