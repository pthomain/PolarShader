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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_FLURRYPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_FLURRYPATTERN_H

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include <memory>

namespace PolarShader {
    /**
     * Stateful advection pattern inspired by a low-resolution flurry/trail simulation.
     *
     * A persistent scalar grid is advected by independent row/column noise profiles,
     * faded every frame, and continuously excited by a moving anti-aliased Lissajous line.
     */
    class FlurryPattern : public UVPattern {
    private:
        struct State;
        struct FlurryFunctor;
        std::shared_ptr<State> state;

    public:
        FlurryPattern(
            uint8_t gridSize = 32,
            Sf16Signal xDrift = constant(50),
            Sf16Signal yDrift = constant(75),
            Sf16Signal amplitude = constant(100),
            Sf16Signal frequency = constant(60),
            Sf16Signal endpointSpeed = constant(40),
            Sf16Signal fade = constant(700)
        );

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;

        UVMap layer(const std::shared_ptr<PipelineContext> &context) const override;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_FLURRYPATTERN_H
