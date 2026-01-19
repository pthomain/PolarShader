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

#include "transforms/base/Transforms.h"
#include "utils/Units.h"
#include "PipelineStep.h"

struct PipelineStep;

namespace PolarShader {
    /**
     * @brief Manages the rendering pipeline for polar effects.
     *
     * Chains transforms across Cartesian and Polar domains and maps palette-ready noise intensities
     * to CRGB. Expects NoiseLayer to output 16-bit intensities in [0..65535]; no normalization is done here.
     * Phase representation is AngleTurnsUQ16_16 (AngleUnitsQ0_16 in the high 16 bits); callers must promote
     * angles before entering the pipeline. Any domain conversion (polar<->cartesian) happens at explicit
     * PipelineStep boundaries.
     */
    class PolarPipeline {
        NoiseLayer sourceLayer;
        CRGBPalette16 palette;
        fl::vector<PipelineStep> steps;
        const char *name;

        static ColourLayer blackLayer(const char *reason);

        static PolarLayer toPolarLayer(const CartesianLayer &layer);

        static CartesianLayer toCartesianLayer(const PolarLayer &layer);

        PolarPipeline(
            NoiseLayer sourceLayer,
            const CRGBPalette16 &palette,
            fl::vector<PipelineStep> steps,
            const char *name
        );

        friend class PolarPipelineBuilder;

    public:
        void advanceFrame(TimeMillis timeInMillis);

        ColourLayer build() const;

        const char *getName() const { return name; }
    };
}

#endif //POLAR_SHADER_PIPELINE_POLARPIPELINE_H
