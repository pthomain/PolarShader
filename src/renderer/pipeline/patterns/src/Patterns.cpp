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
#include "renderer/pipeline/patterns/FlowFieldPattern.h"
#include "renderer/pipeline/patterns/TransportPattern.h"
#include "renderer/pipeline/patterns/NoisePattern.h"
#include "renderer/pipeline/patterns/TilingPattern.h"
#include "renderer/pipeline/patterns/ReactionDiffusionPattern.h"
#include "renderer/pipeline/patterns/ConwayPattern.h"
#include "renderer/pipeline/patterns/CyclicCAPattern.h"
#include "renderer/pipeline/patterns/BriansBrainPattern.h"
#include "renderer/pipeline/patterns/LifeVariantPattern.h"
#include "renderer/pipeline/patterns/ElementaryCAPattern.h"
#include "renderer/pipeline/patterns/MatrixRainPattern.h"
#include "renderer/pipeline/patterns/RipplePattern.h"
#include "renderer/pipeline/patterns/ForestFirePattern.h"
#include "renderer/pipeline/patterns/WireWorldPattern.h"
#include "renderer/pipeline/patterns/LangtonAntPattern.h"
#include "renderer/pipeline/patterns/RasterReactionDiffusionPattern.h"
#include "renderer/pipeline/patterns/XORPattern.h"
#include "renderer/pipeline/patterns/SpiralPattern.h"
#include "renderer/pipeline/patterns/AnnuliPattern.h"

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

    std::unique_ptr<UVPattern> conwayPattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint16_t densityPermille
    ) {
        return std::make_unique<ConwayPattern>(stepIntervalMs, seed, densityPermille);
    }

    std::unique_ptr<UVPattern> xorPattern(
        uint8_t gridSize,
        uint16_t speed
    ) {
        return std::make_unique<XORPattern>(gridSize, speed);
    }

    std::unique_ptr<UVPattern> cyclicCAPattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint8_t numStates,
        uint8_t threshold
    ) {
        return std::make_unique<CyclicCAPattern>(stepIntervalMs, seed, numStates, threshold);
    }

    std::unique_ptr<UVPattern> briansBrainPattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint16_t densityPermille
    ) {
        return std::make_unique<BriansBrainPattern>(stepIntervalMs, seed, densityPermille);
    }

    std::unique_ptr<UVPattern> lifeVariantPattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint16_t densityPermille,
        LifeVariantPattern::Rule rule
    ) {
        return std::make_unique<LifeVariantPattern>(stepIntervalMs, seed, densityPermille, rule);
    }

    std::unique_ptr<UVPattern> elementaryCAPattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint8_t rule
    ) {
        return std::make_unique<ElementaryCAPattern>(stepIntervalMs, seed, rule);
    }

    std::unique_ptr<UVPattern> matrixRainPattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint8_t fadeAmount
    ) {
        return std::make_unique<MatrixRainPattern>(stepIntervalMs, seed, fadeAmount);
    }

    std::unique_ptr<UVPattern> ripplePattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint8_t damping
    ) {
        return std::make_unique<RipplePattern>(stepIntervalMs, seed, damping);
    }

    std::unique_ptr<UVPattern> forestFirePattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint16_t growthPermille,
        uint16_t lightningPermille
    ) {
        return std::make_unique<ForestFirePattern>(stepIntervalMs, seed, growthPermille, lightningPermille);
    }

    std::unique_ptr<UVPattern> wireWorldPattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint16_t densityPermille
    ) {
        return std::make_unique<WireWorldPattern>(stepIntervalMs, seed, densityPermille);
    }

    std::unique_ptr<UVPattern> langtonAntPattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint16_t antCount
    ) {
        return std::make_unique<LangtonAntPattern>(stepIntervalMs, seed, antCount);
    }

    std::unique_ptr<UVPattern> rasterReactionDiffusionPattern(
        uint8_t preset,
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint16_t iterationsPerStep
    ) {
        return std::make_unique<RasterReactionDiffusionPattern>(preset, stepIntervalMs, seed, iterationsPerStep);
    }

    std::unique_ptr<UVPattern> flowFieldPattern(
        uint8_t gridSize,
        uint8_t dotCount,
        FlowFieldPattern::EmitterMode mode,
        Sf16Signal xDrift,
        Sf16Signal yDrift,
        Sf16Signal amplitude,
        Sf16Signal frequency,
        Sf16Signal endpointSpeed,
        Sf16Signal halfLife,
        Sf16Signal orbitSpeed,
        Sf16Signal orbitRadius
    ) {
        return std::make_unique<FlowFieldPattern>(
            gridSize,
            dotCount,
            mode,
            std::move(xDrift),
            std::move(yDrift),
            std::move(amplitude),
            std::move(frequency),
            std::move(endpointSpeed),
            std::move(halfLife),
            std::move(orbitSpeed),
            std::move(orbitRadius)
        );
    }

    std::unique_ptr<UVPattern> transportPattern(
        uint8_t gridSize,
        TransportPattern::TransportMode mode,
        Sf16Signal radialSpeed,
        Sf16Signal angularSpeed,
        Sf16Signal halfLife,
        Sf16Signal emitterSpeed,
        bool velocityGlow
    ) {
        return std::make_unique<TransportPattern>(
            gridSize,
            mode,
            std::move(radialSpeed),
            std::move(angularSpeed),
            std::move(halfLife),
            std::move(emitterSpeed),
            velocityGlow
        );
    }

    std::unique_ptr<UVPattern> spiralPattern(
        uint8_t armCount,
        bool clockwise,
        Sf16Signal tightness,
        Sf16Signal armThickness,
        Sf16Signal rotationSpeed
    ) {
        return std::make_unique<SpiralPattern>(
            armCount,
            clockwise,
            std::move(tightness),
            std::move(armThickness),
            std::move(rotationSpeed)
        );
    }

    std::unique_ptr<UVPattern> annuliPattern(
        uint8_t ringCount,
        uint8_t slicesPerRing,
        bool reverse,
        uint16_t stepIntervalMs,
        uint16_t holdMs
    ) {
        return std::make_unique<AnnuliPattern>(
            ringCount,
            slicesPerRing,
            reverse,
            stepIntervalMs,
            holdMs
        );
    }

    std::unique_ptr<UVPattern> flurryPattern(
        uint8_t gridSize,
        uint8_t lineCount,
        FlurryPattern::Shape shape,
        Sf16Signal xDrift,
        Sf16Signal yDrift,
        Sf16Signal amplitude,
        Sf16Signal frequency,
        Sf16Signal endpointSpeed,
        Sf16Signal fade
    ) {
        return std::make_unique<FlurryPattern>(
            gridSize,
            lineCount,
            shape,
            std::move(xDrift),
            std::move(yDrift),
            std::move(amplitude),
            std::move(frequency),
            std::move(endpointSpeed),
            std::move(fade)
        );
    }
}
