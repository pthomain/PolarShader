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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_FLOWFIELDPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_FLOWFIELDPATTERN_H

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include <memory>

namespace PolarShader {
    /**
     * Stateful advection pattern with orbital dot emitters and noise punch.
     *
     * A persistent scalar grid is advected by independent row/column noise profiles,
     * faded each frame using a framerate-independent half-life, and continuously
     * excited by configurable emitters (Lissajous line, orbital dots, or both).
     *
     * Ported from the FastLED FlowField effect (concept by Stefan Petrick,
     * initial C++ by 4wheeljive). Shares grid infrastructure with FlurryPattern
     * via GridUtils.h.
     */
    class FlowFieldPattern : public UVPattern {
    private:
        struct State;
        struct FlowFieldFunctor;
        std::shared_ptr<State> state;

    public:
        enum class EmitterMode : uint8_t {
            Lissajous,
            Dots,
            Both
        };

        enum class NoisePunchShape : uint8_t {
            HalfSine,
            Gaussian
        };

        FlowFieldPattern(
            uint8_t gridSize = 32,
            uint8_t dotCount = 3,
            EmitterMode mode = EmitterMode::Both,
            Sf16Signal xDrift = constant(50),
            Sf16Signal yDrift = constant(75),
            Sf16Signal amplitude = constant(100),
            Sf16Signal frequency = constant(60),
            Sf16Signal endpointSpeed = constant(500),
            Sf16Signal halfLife = constant(600),
            Sf16Signal orbitSpeed = constant(300),
            Sf16Signal orbitRadius = constant(500)
        );

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;

        UVMap layer(const std::shared_ptr<PipelineContext> &context) const override;

        /// Trigger a noise punch on the X axis (columns).
        void triggerNoisePunchX(
            fl::s16x16 center, fl::s16x16 width, sf16 amplitude,
            NoisePunchShape shape = NoisePunchShape::HalfSine
        );

        /// Trigger a noise punch on the Y axis (rows).
        void triggerNoisePunchY(
            fl::s16x16 center, fl::s16x16 width, sf16 amplitude,
            NoisePunchShape shape = NoisePunchShape::HalfSine
        );
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_FLOWFIELDPATTERN_H
