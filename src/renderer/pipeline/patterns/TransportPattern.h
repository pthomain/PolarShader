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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_TRANSPORTPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_TRANSPORTPATTERN_H

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include <memory>

namespace PolarShader {
    /**
     * Stateful 2D advection pattern with pluggable transport vector fields.
     *
     * A persistent scalar grid is advected per-pixel using backward bilinear
     * sampling along analytically computed displacement vectors. Different
     * TransportMode values select the vector field: polar spirals, radial
     * sink/source, shockwaves, attractor fields, and more.
     *
     * Shares grid infrastructure (drawing, fade, emitter injection) with
     * FlurryPattern and FlowFieldPattern via GridUtils.h.
     */
    class TransportPattern : public UVPattern {
    private:
        struct State;
        struct TransportFunctor;
        std::shared_ptr<State> state;

    public:
        enum class TransportMode : uint8_t {
            SpiralInward,
            SpiralOutward,
            RadialSink,
            RadialSource,
            RotatingSwirl,
            PolarPulse,
            Shockwave,
            FractalSpiral,
            SquareSpiral,
            AttractorField
        };

        TransportPattern(
            uint8_t gridSize = 32,
            TransportMode mode = TransportMode::SpiralInward,
            Sf16Signal radialSpeed = constant(300),
            Sf16Signal angularSpeed = constant(200),
            Sf16Signal halfLife = constant(600),
            Sf16Signal emitterSpeed = constant(500),
            bool velocityGlow = false
        );

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;

        UVMap layer(const std::shared_ptr<PipelineContext> &context) const override;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_TRANSPORTPATTERN_H
