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
#include <renderer/pipeline/patterns/FlowFieldPattern.h>
#include <renderer/pipeline/patterns/TransportPattern.h>
#include <renderer/pipeline/patterns/TilingPattern.h>
#include <renderer/pipeline/patterns/ReactionDiffusionPattern.h>
#include <renderer/pipeline/patterns/ConwayPattern.h>
#include <renderer/pipeline/patterns/CyclicCAPattern.h>
#include <renderer/pipeline/patterns/BriansBrainPattern.h>
#include <renderer/pipeline/patterns/LifeVariantPattern.h>
#include <renderer/pipeline/patterns/ElementaryCAPattern.h>
#include <renderer/pipeline/patterns/MatrixRainPattern.h>
#include <renderer/pipeline/patterns/RipplePattern.h>
#include <renderer/pipeline/patterns/ForestFirePattern.h>
#include <renderer/pipeline/patterns/WireWorldPattern.h>
#include <renderer/pipeline/patterns/LangtonAntPattern.h>
#include <renderer/pipeline/patterns/RasterReactionDiffusionPattern.h>
#include <renderer/pipeline/patterns/XORPattern.h>
#include <renderer/pipeline/patterns/SpiralPattern.h>
#include <renderer/pipeline/patterns/AnnuliPattern.h>
#include <renderer/pipeline/patterns/PaletteGlowPattern.h>
#include <renderer/pipeline/patterns/patternflow/PatternFlow.h>
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

    // Display-native Conway's Game of Life. Uses logical raster x/y.
    std::unique_ptr<UVPattern> conwayPattern(
        uint16_t stepIntervalMs = 250,
        uint16_t seed = 0,
        uint16_t densityPermille = 350
    );

    // Cyclic cellular automaton (Greenberg-Hastings); rotating spiral waves.
    std::unique_ptr<UVPattern> cyclicCAPattern(
        uint16_t stepIntervalMs = 120,
        uint16_t seed = 0,
        uint8_t numStates = 8,
        uint8_t threshold = 1
    );

    // Brian's Brain 3-state automaton (off/firing/dying).
    std::unique_ptr<UVPattern> briansBrainPattern(
        uint16_t stepIntervalMs = 90,
        uint16_t seed = 0,
        uint16_t densityPermille = 300
    );

    // Life-like automata with alternate birth/survival rules.
    std::unique_ptr<UVPattern> lifeVariantPattern(
        uint16_t stepIntervalMs = 200,
        uint16_t seed = 0,
        uint16_t densityPermille = 350,
        LifeVariantPattern::Rule rule = LifeVariantPattern::Rule::HighLife
    );

    // One-dimensional (Wolfram) cellular automaton scroller.
    std::unique_ptr<UVPattern> elementaryCAPattern(
        uint16_t stepIntervalMs = 90,
        uint16_t seed = 0,
        uint8_t rule = 30
    );

    // Falling "digital rain" columns with fading trails.
    std::unique_ptr<UVPattern> matrixRainPattern(
        uint16_t stepIntervalMs = 60,
        uint16_t seed = 0,
        uint8_t fadeAmount = 40
    );

    // Integer water-ripple wave simulation.
    std::unique_ptr<UVPattern> ripplePattern(
        uint16_t stepIntervalMs = 40,
        uint16_t seed = 0,
        uint8_t damping = 6
    );

    // Drossel-Schwabl forest-fire model (empty/tree/burning).
    std::unique_ptr<UVPattern> forestFirePattern(
        uint16_t stepIntervalMs = 120,
        uint16_t seed = 0,
        uint16_t growthPermille = 40,
        uint16_t lightningPermille = 2
    );

    // Wireworld electron-transport automaton.
    std::unique_ptr<UVPattern> wireWorldPattern(
        uint16_t stepIntervalMs = 100,
        uint16_t seed = 0,
        uint16_t densityPermille = 500
    );

    // Langton's ant trail on a toroidal binary grid.
    std::unique_ptr<UVPattern> langtonAntPattern(
        uint16_t stepIntervalMs = 30,
        uint16_t seed = 0,
        uint16_t antCount = 1
    );

    // Display-native Gray-Scott reaction-diffusion (raster grid).
    std::unique_ptr<UVPattern> rasterReactionDiffusionPattern(
        uint8_t preset = 0,
        uint16_t stepIntervalMs = 33,
        uint16_t seed = 0,
        uint16_t iterationsPerStep = 4
    );

    // Animated (x XOR y) "munching squares" texture.
    std::unique_ptr<UVPattern> xorPattern(
        uint8_t gridSize = 16,
        uint16_t speed = 40
    );

    // Persistent advected trail field with orbital dots, noise punch, and half-life fade.
    std::unique_ptr<UVPattern> flowFieldPattern(
        uint8_t gridSize = 32,
        uint8_t dotCount = 3,
        FlowFieldPattern::EmitterMode mode = FlowFieldPattern::EmitterMode::Both,
        Sf16Signal xDrift = constant(50),
        Sf16Signal yDrift = constant(75),
        Sf16Signal amplitude = constant(100),
        Sf16Signal frequency = constant(60),
        Sf16Signal endpointSpeed = constant(500),
        Sf16Signal halfLife = constant(600),
        Sf16Signal orbitSpeed = constant(300),
        Sf16Signal orbitRadius = constant(500)
    );

    // 2D backward-advected transport field with pluggable vector modes.
    std::unique_ptr<UVPattern> transportPattern(
        uint8_t gridSize = 32,
        TransportPattern::TransportMode mode = TransportPattern::TransportMode::SpiralInward,
        Sf16Signal radialSpeed = constant(300),
        Sf16Signal angularSpeed = constant(200),
        Sf16Signal halfLife = constant(600),
        Sf16Signal emitterSpeed = constant(500),
        bool velocityGlow = false
    );

    // Rotating Archimedean spiral. Stateless per-pixel (theta - tightness*r - phase) formula.
    std::unique_ptr<UVPattern> spiralPattern(
        uint8_t armCount = 2,
        bool clockwise = true,
        Sf16Signal tightness = constant(720),
        Sf16Signal armThickness = constant(500),
        Sf16Signal rotationSpeed = constant(300)
    );

    // Concentric annuli sweep-fill. Polar reinterpretation of CD77 box-fill.
    std::unique_ptr<UVPattern> annuliPattern(
        uint8_t ringCount = 8,
        uint8_t slicesPerRing = 32,
        bool reverse = false,
        uint16_t stepIntervalMs = 80,
        uint16_t holdMs = 800
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

    // ShaderToy-style recursive radial glow using the IQ cosine palette.
    std::unique_ptr<UVPattern> paletteGlowPattern();
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_H
