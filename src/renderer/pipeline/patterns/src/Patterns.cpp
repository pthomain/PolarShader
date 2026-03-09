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

#include "renderer/pipeline/patterns/Patterns.h"
#include "renderer/pipeline/patterns/FlurryPattern.h"
#include "renderer/pipeline/patterns/NoisePattern.h"
#include "renderer/pipeline/patterns/TilingPattern.h"
#include "renderer/pipeline/patterns/ReactionDiffusionPattern.h"

namespace PolarShader {
    std::unique_ptr<UVPattern> worleyPattern(
        fl::s24x8 cellSize,
        WorleyAliasing aliasingMode
    ) {
        return std::make_unique<WorleyPattern>(cellSize, aliasingMode);
    }

    std::unique_ptr<UVPattern> voronoiPattern(
        fl::s24x8 cellSize,
        WorleyAliasing aliasingMode
    ) {
        return std::make_unique<VoronoiPattern>(cellSize, aliasingMode);
    }

    std::unique_ptr<UVPattern> noisePattern(Sf16Signal depthSpeedSignal) {
        return std::make_unique<NoisePattern>(
            NoisePattern::NoiseType::Basic,
            4,
            std::move(depthSpeedSignal)
        );
    }

    std::unique_ptr<UVPattern> fbmNoisePattern(fl::u8 octaves) {
        return std::make_unique<NoisePattern>(NoisePattern::NoiseType::FBM, octaves);
    }

    std::unique_ptr<UVPattern> turbulenceNoisePattern() {
        return std::make_unique<NoisePattern>(NoisePattern::NoiseType::Turbulence);
    }

    std::unique_ptr<UVPattern> ridgedNoisePattern() {
        return std::make_unique<NoisePattern>(NoisePattern::NoiseType::Ridged);
    }

    std::unique_ptr<UVPattern> tilingPattern(
        uint16_t cellSize,
        uint8_t colorCount,
        TilingPattern::TileShape shape
    ) {
        return std::make_unique<TilingPattern>(cellSize, colorCount, shape);
    }

    std::unique_ptr<UVPattern> reactionDiffusionPattern(
        ReactionDiffusionPattern::Preset preset,
        uint8_t width,
        uint8_t height,
        uint8_t stepsPerFrame
    ) {
        return std::make_unique<ReactionDiffusionPattern>(preset, width, height, stepsPerFrame);
    }

    std::unique_ptr<UVPattern> flurryPattern(
        uint8_t gridSize,
        Sf16Signal xDrift,
        Sf16Signal xAmplitude,
        Sf16Signal xFrequency,
        Sf16Signal yDrift,
        Sf16Signal yAmplitude,
        Sf16Signal yFrequency,
        Sf16Signal endpointSpeed,
        Sf16Signal fade
    ) {
        return std::make_unique<FlurryPattern>(
            gridSize,
            std::move(xDrift),
            std::move(xAmplitude),
            std::move(xFrequency),
            std::move(yDrift),
            std::move(yAmplitude),
            std::move(yFrequency),
            std::move(endpointSpeed),
            std::move(fade)
        );
    }
}
