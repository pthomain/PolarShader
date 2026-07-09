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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_PALETTEGLOWPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_PALETTEGLOWPATTERN_H

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include "renderer/pipeline/signals/Signals.h"
#include <memory>

namespace PolarShader {
    /**
     * ShaderToy-style recursive radial glow using the IQ cosine palette.
     *
     * Port of the fragment shader from https://www.shadertoy.com/view/mtyGWy
     * and https://iquilezles.org/articles/palettes/ into the existing UV
     * pattern pipeline. It emits full RGB samples through UVLayer::Rgb.
     */
    class PaletteGlowPattern : public UVPattern {
    public:
        explicit PaletteGlowPattern(Sf16Signal speed = constant(1000));

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;

        UVMap layer(const std::shared_ptr<PipelineContext> &context) const override;

        UVLayer uvLayer(const std::shared_ptr<PipelineContext> &context) const override;

    private:
        struct State;
        struct Functor;
        std::shared_ptr<State> state;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_PALETTEGLOWPATTERN_H
