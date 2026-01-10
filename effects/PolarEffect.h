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
#include "polar/pipeline/PolarPipeline.h"
#include "polar/pipeline/decorators/KaleidoscopeDecorator.h"
#include "polar/pipeline/decorators/VortexDecorator.h"
#include "polar/pipeline/decorators/RotationDecorator.h"

namespace LEDSegments {
    class PolarEffect : public Effect<PolarEffect> {
        PolarPipeline pipeline;
        ColourLayer finalLayer;

        RotationDecorator rotationDecorator;
        KaleidoscopeDecorator kaleidoscopeDecorator;
        VortexDecorator vortexDecorator;

        inline CRGB blendLayers(
            uint16_t angle,
            fract16 radius,
            unsigned long timeInMillis,
            const ColourLayer &layer
        );

        inline CRGB blendLayers(
            uint16_t angle,
            fract16 radius,
            unsigned long timeInMillis,
            const fl::vector<ColourLayer> &layers
        );

    public:
        explicit PolarEffect(const RenderableContext &context);

        void fillSegmentArray(
            CRGB *segmentArray,
            uint16_t segmentSize,
            uint16_t segmentIndex,
            fract16 progress,
            unsigned long timeInMillis
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
