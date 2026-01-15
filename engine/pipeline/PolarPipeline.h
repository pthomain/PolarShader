//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2023 Pierre Thomain

/*
 * This file is part of LED Segments.
 *
 * LED Segments is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LED Segments is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LED Segments. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LED_SEGMENTS_PIPELINE_POLARPIPELINE_H
#define LED_SEGMENTS_PIPELINE_POLARPIPELINE_H

#include "transforms/base/Transforms.h"
#ifdef ARDUINO
#include "Arduino.h"
#endif
#include "PolarUtils.h"
#include "utils/Units.h"
#include <memory>

#include "PipelineStep.h"

struct PipelineStep;

namespace LEDSegments {
    /**
     * @brief Manages the rendering pipeline for polar effects.
     *
     * Chains transforms across Cartesian and Polar domains and maps palette-ready noise intensities
     * to CRGB. Expects NoiseLayer to output 16-bit intensities in [0..65535]; no normalization is done here.
     * Phase representation is PhaseTurnsUQ16_16 (AngleTurns16 in the high 16 bits); callers must promote
     * angles before entering the pipeline. Any domain conversion (polar<->cartesian) happens at explicit
     * PipelineStep boundaries.
     */
    class PolarPipeline {
        NoiseLayer sourceLayer;
        CRGBPalette16 palette;
        fl::vector<PipelineStep> steps;

        static ColourLayer blackLayer(const char *reason);

        static PolarLayer toPolarLayer(const CartesianLayer &layer);

        static CartesianLayer toCartesianLayer(const PolarLayer &layer);

        PolarPipeline(
            NoiseLayer sourceLayer,
            const CRGBPalette16 &palette,
            fl::vector<PipelineStep> steps
        );

        friend class PolarPipelineBuilder;

    public:
        void advanceFrame(Units::TimeMillis timeInMillis);

        ColourLayer build() const;
    };
}

#endif //LED_SEGMENTS_PIPELINE_POLARPIPELINE_H
