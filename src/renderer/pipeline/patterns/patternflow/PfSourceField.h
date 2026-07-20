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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFSOURCEFIELD_H
#define POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFSOURCEFIELD_H

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include <memory>

namespace PolarShader {
    /**
     * Ripple-tank interference field from N drifting point emitters.
     *
     * Original PolarShader code (no PatternFlow adaptation). Each of
     * `sourceCount` emitters sits on a slowly rotating ring and radiates a
     * travelling circular wave sin(k*|p - p_i| - w*t). The waves are summed and
     * normalised, giving the classic multi-source interference pattern: bright
     * crests where wavefronts reinforce, dark troughs where they cancel.
     *
     * Intrinsic parameters only (per the PatternFlow design rule). Global
     * scale -> ZoomTransform, colour/hue -> PaletteTransform, tiling ->
     * TilingTransform, drift/rotation live in the preset transform stack.
     *
     * Parameter ranges (signals sampled in magnitudeRange() -> [0, 1]):
     *   phaseSpeed [0,1] -> internal animation + emitter drift rate.
     *   warp       [0,1] -> spatial frequency of the waves (ring density).
     *   thickness  [0,1] -> width of the lit crest band.
     * sourceCount is structural: number of emitters (clamped to 1..8).
     */
    class PfSourceField : public UVPattern {
    public:
        explicit PfSourceField(
            uint8_t sourceCount = 3,
            S0x16Signal phaseSpeed = constant(500),
            S0x16Signal warp = constant(500),
            S0x16Signal thickness = constant(500)
        );

        void advanceFrame(u0x16 progress, TimeMillis elapsedMs) override;

        UVMap layer(const std::shared_ptr<PipelineContext> &context) const override;

        bool emitsColour() const override;

        UVColourMap colourLayer(const std::shared_ptr<PipelineContext> &context) const override;

    private:
        struct State;
        struct Functor;
        std::shared_ptr<State> state;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFSOURCEFIELD_H
