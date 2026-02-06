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

#ifndef POLAR_SHADER_PIPELINE_POLARPIPELINE_H
#define POLAR_SHADER_PIPELINE_POLARPIPELINE_H

#include "patterns/BasePattern.h"
#include "PipelineContext.h"
#include "PipelineStep.h"
#include "renderer/pipeline/maths/CartesianMaths.h"
#include "renderer/pipeline/signals/Accumulators.h"
#include <memory>

namespace PolarShader {
    /**
     * @brief Manages the rendering pipeline for polar effects.
     *
     * Chains transforms across Cartesian and Polar domains and maps palette-ready pattern intensities
     * to CRGB. Expects the base pattern to output 16-bit intensities in [0..65535]; no normalization is done here.
     * Angles are represented as 16-bit turns (Q0.16). Cartesian coords are Q24.8. Any domain conversion (polar<->cartesian)
     * happens at explicit PipelineStep boundaries.
     */
    class PolarPipeline {
        std::unique_ptr<BasePattern> pattern;
        CRGBPalette16 palette;
        fl::vector<PipelineStep> steps;
        const char *name;
        std::shared_ptr<PipelineContext> context;
        DepthSignal depthSignal;

        static ColourLayer blackLayer(const char *reason);

        static CRGB mapPalette(
            const CRGBPalette16 &palette,
            PatternNormU16 value,
            const std::shared_ptr<PipelineContext> &context
        );

        PolarPipeline(
            std::unique_ptr<BasePattern> pattern,
            const CRGBPalette16 &palette,
            fl::vector<PipelineStep> steps,
            const char *name,
            std::shared_ptr<PipelineContext> context,
            DepthSignal depthSignal
        );

        friend class PolarPipelineBuilder;

    public:
        void advanceFrame(TimeMillis timeInMillis);

        ColourLayer build() const;

        const char *getName() const { return name; }
    };
}

#endif //POLAR_SHADER_PIPELINE_POLARPIPELINE_H
