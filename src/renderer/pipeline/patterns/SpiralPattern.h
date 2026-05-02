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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_SPIRALPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_SPIRALPATTERN_H

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include <memory>

namespace PolarShader {
    /**
     * Stateless rotating Archimedean spiral pattern.
     *
     * Per-pixel formula: intensity = cos(pi * |angle - tightness*r - phase| / halfArm)
     * mapped through a cosine falloff window, with a centre-to-edge brightness gradient.
     * Tightness, arm thickness, and rotation speed are all Sf16Signal-driven.
     *
     * Parameter ranges:
     *   tightness     [0, 1] -> [0, 2] full turns of winding from centre to edge
     *   armThickness  [0, 1] -> fraction of arm-spacing (clamped internally to [0.05, 0.90])
     *   rotationSpeed [0, 1] -> [0, 1] full turns per second
     *
     * The clockwise flag controls the sign of phase advancement; the visually-perceived
     * rotation direction depends on the display's UV orientation. Flip if it looks wrong.
     */
    class SpiralPattern : public UVPattern {
    private:
        struct State;
        struct UVSpiralFunctor;
        std::shared_ptr<State> state;

    public:
        SpiralPattern(
            uint8_t armCount = 2,
            bool clockwise = true,
            Sf16Signal tightness = constant(720),
            Sf16Signal armThickness = constant(500),
            Sf16Signal rotationSpeed = constant(300)
        );

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;

        UVMap layer(const std::shared_ptr<PipelineContext> &context) const override;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_SPIRALPATTERN_H
