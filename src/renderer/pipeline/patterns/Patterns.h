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
#include <renderer/pipeline/patterns/WorleyConstants.h>
#include <renderer/pipeline/patterns/WorleyPatterns.h>
#include <renderer/pipeline/patterns/FlurryPattern.h>
#include <renderer/pipeline/patterns/TilingPattern.h>
#include <renderer/pipeline/patterns/ReactionDiffusionPattern.h>
#include <renderer/pipeline/signals/SignalTypes.h>

#include "renderer/pipeline/signals/Signals.h"

namespace PolarShader {
    // Pattern factory helpers. These mirror the Presets API for quick creation.

    // Nearest-distance Worley field (F1).
    std::unique_ptr<UVPattern> worleyPattern(
        fl::s24x8 cellSize = fl::s24x8::from_raw(WorleyCellUnit),
        WorleyAliasing aliasingMode = WorleyAliasing::Fast
    );

    // Hash-based Voronoi cell ID mapped to 0..65535.
    std::unique_ptr<UVPattern> voronoiPattern(
        fl::s24x8 cellSize = fl::s24x8::from_raw(WorleyCellUnit),
        WorleyAliasing aliasingMode = WorleyAliasing::Fast
    );

    // Base noise pattern (inoise16) with signed cartesian input.
    std::unique_ptr<UVPattern> noisePattern(Sf16Signal depthSpeedSignal = cRandom());

    // fBm noise pattern with configurable octaves (default 4).
    std::unique_ptr<UVPattern> fbmNoisePattern(
        fl::u8 octaves = 4
    );

    // Turbulence noise (absolute-value style).
    std::unique_ptr<UVPattern> turbulenceNoisePattern();

    // Ridged noise (inverted turbulence).
    std::unique_ptr<UVPattern> ridgedNoisePattern();

    // Shape-aware tiling with N-colouring (no adjacent matches when colorCount >= 3).
    std::unique_ptr<UVPattern> tilingPattern(
        uint16_t cellSize = 1,
        uint8_t colorCount = 4,
        TilingPattern::TileShape shape = TilingPattern::TileShape::HEXAGON
    );

    // Gray-Scott reaction-diffusion simulation.
    // Runs a stateful per-cell simulation; V concentration is bilinearly sampled.
    std::unique_ptr<UVPattern> reactionDiffusionPattern(
        ReactionDiffusionPattern::Preset preset = ReactionDiffusionPattern::Preset::Coral,
        uint8_t width = 20,
        uint8_t height = 20,
        uint8_t stepsPerFrame = 4
    );

    // Persistent advected trail field driven by 1D noise profiles.
    std::unique_ptr<UVPattern> flurryPattern(
        uint8_t gridSize = 32,
        uint8_t lineCount = 1,
        FlurryPattern::Shape shape = FlurryPattern::Shape::Line,
        Sf16Signal xDrift = constant(50),
        Sf16Signal yDrift = constant(75),
        Sf16Signal amplitude = constant(100),
        Sf16Signal frequency = constant(60),
        Sf16Signal endpointSpeed = constant(500),
        Sf16Signal fade = constant(700)
    );
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_H
