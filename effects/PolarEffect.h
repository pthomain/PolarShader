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

#ifndef LED_SEGMENTS_POLAREFFECT_H
#define LED_SEGMENTS_POLAREFFECT_H

#include "engine/render/renderable/BaseRenderableFactory.h"
#include "engine/render/renderable/TypedRenderable.h"
#include "polar/pipeline/PolarPipelineBuilder.h"
#include "polar/pipeline/utils/Units.h"

namespace LEDSegments {
    using namespace Units;
    /**
     * PolarEffect builds a PolarPipeline from the current palette and renders using AngleTurnsUQ16_16 angles
     * (AngleUnitsQ0_16 stored in the upper 16 bits). Callers must promote AngleUnitsQ0_16 -> AngleTurnsUQ16_16
     * once before entering the pipeline. The pipeline assumes incoming noise layers already produce
     * palette-ready intensities in [0..65535]; no normalization occurs past the source layer.
     */
    class PolarEffect : public Effect<PolarEffect> {
        PolarPipeline pipeline;
        ColourLayer colourLayer;

        static CRGB blendLayers(
            Units::AngleTurnsUQ16_16 angle_q16,
            FracQ0_16 radius,
            const ColourLayer &layer
        );

        CRGB blendLayers(
            Units::AngleTurnsUQ16_16 angle_q16,
            FracQ0_16 radius,
            const fl::vector<ColourLayer> &layers
        );

    public:
        explicit PolarEffect(const RenderableContext &context);

        void fillSegmentArray(
            CRGB *segmentArray,
            uint16_t segmentSize,
            uint16_t segmentIndex,
            FracQ0_16 progress,
            TimeMillis timeInMillis
        ) override;


        static constexpr const char *name() { return "PolarEffect"; }
        static RenderableFactoryRef<CRGB> factory;
    };

    class PolarEffectFactory : public RenderableFactory<PolarEffectFactory, PolarEffect, CRGB> {
    public:
        static Params declareParams() {
            return {};
        }
    };
} // namespace LEDSegments

#endif //LED_SEGMENTS_POLAREFFECT_H
