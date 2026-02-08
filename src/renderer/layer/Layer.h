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

#ifndef POLAR_SHADER_PIPELINE_LAYER_H
#define POLAR_SHADER_PIPELINE_LAYER_H

#ifdef ARDUINO
#include "FastLED.h"
#else
#include "native/FastLED.h"
#endif

#include "renderer/pipeline/maths/units/Units.h"
#include "../pipeline/patterns/base/UVPattern.h"
#include "../pipeline/PipelineContext.h"
#include "../pipeline/PipelineStep.h"
#include "renderer/pipeline/signals/Accumulators.h"
#include "renderer/pipeline/transforms/base/Layers.h"
#include <memory>

namespace PolarShader {
    enum class BlendMode {
        Normal,
        Add,
        Multiply,
        Screen
    };

    /**
     * @brief Manages the rendering pipeline for polar effects.
     *
     * Chains transforms across Cartesian and Polar domains and maps palette-ready pattern intensities
     * to CRGB. Expects the base pattern to output 16-bit intensities in [0..65535]; no normalization is done here.
     * Angles are represented as 16-bit turns (Q0.16). Cartesian coords are Q24.8. Any domain conversion (polar<->cartesian)
     * happens at explicit PipelineStep boundaries.
     */
    class Layer {
        std::unique_ptr<UVPattern> pattern;
        CRGBPalette16 palette;
        fl::vector<PipelineStep> steps;
        const char *name;
        std::shared_ptr<PipelineContext> context;
        DepthSignal depthSignal;
        
        FracQ0_16 alpha{0xFFFFu};
        BlendMode blendMode{BlendMode::Normal};

        static ColourMap blackLayer(const char *reason);

        static CRGB mapPalette(
            const CRGBPalette16 &palette,
            PatternNormU16 value,
            const std::shared_ptr<PipelineContext> &context
        );

        Layer(
            std::unique_ptr<UVPattern> pattern,
            const CRGBPalette16 &palette,
            fl::vector<PipelineStep> steps,
            const char *name,
            std::shared_ptr<PipelineContext> context,
            DepthSignal depthSignal,
            FracQ0_16 alpha = FracQ0_16(0xFFFFu),
            BlendMode blendMode = BlendMode::Normal
        );

        friend class LayerBuilder;

    public:
        void advanceFrame(FracQ0_16 progress, TimeMillis elapsedMs);

        ColourMap build() const;

        const char *getName() const { return name; }

        FracQ0_16 getAlpha() const { return alpha; }
        BlendMode getBlendMode() const { return blendMode; }
    };
}

#endif //POLAR_SHADER_PIPELINE_LAYER_H
