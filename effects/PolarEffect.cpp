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

#include "PolarEffect.h"
#include "../pipeline/PolarUtils.h"

namespace LEDSegments {
    static const PolarEffectFactory factoryInstance;
    RenderableFactoryRef<CRGB> PolarEffect::factory = &factoryInstance;

    PolarEffect::PolarEffect(
        const RenderableContext &context
    ) : Effect(context),
        pipeline(
            NoiseValue(400, 100),
            ConstantValue(0), //NoiseValue(1000, 200),
            ConstantValue(0) //NoiseValue(1000)
        ),
        rotationDecorator(ConstantValue(0)),
        kaleidoscopeDecorator(1, false, true),
        vortexDecorator(ConstantValue(0)) {
        pipeline.addPolarDecorator(&rotationDecorator);
        pipeline.addPolarDecorator(&vortexDecorator);
        pipeline.addPolarDecorator(&kaleidoscopeDecorator);

        finalLayer = pipeline.build(noiseLayer, context.palette.palette);
    }

    /**
 * @brief Evaluates a single ColorLayer. This is the fast-path overload.
 */
    inline CRGB PolarEffect::blendLayers(
        uint16_t angle,
        fract16 radius,
        unsigned long timeInMillis,
        const ColourLayer &layer
    ) {
        return layer(angle, radius, timeInMillis);
    }

    /**
     * @brief Blends a stack of ColorLayers for a specific pixel using additive blending.
     * Handles HDR blending by accumulating in 16-bit and scaling down if overflow occurs.
     */
    inline CRGB PolarEffect::blendLayers(
        uint16_t angle,
        fract16 radius,
        unsigned long timeInMillis,
        const fl::vector<ColourLayer> &layers
    ) {
        if (layers.empty()) return CRGB::Black;

        // Fast path for a single layer, bypassing HDR accumulation.
        if (layers.size() == 1) {
            return layers[0](angle, radius, timeInMillis);
        }

        CRGB16 blended16;

        for (const auto &layer: layers) {
            CRGB value = layer(
                angle,
                radius,
                timeInMillis
            );
            blended16 += value;
        }

        uint16_t max_val = max(blended16.r, max(blended16.g, blended16.b));

        if (max_val > UINT8_MAX) {
            uint16_t scale = (UINT8_MAX * UINT16_MAX) / max_val;
            blended16.r = scale16(blended16.r, scale);
            blended16.g = scale16(blended16.g, scale);
            blended16.b = scale16(blended16.b, scale);
        }

        return CRGB(blended16.r, blended16.g, blended16.b);
    }

    void PolarEffect::fillSegmentArray(
        CRGB *segmentArray,
        uint16_t segmentSize,
        uint16_t segmentIndex,
        fract16 progress,
        unsigned long timeInMillis
    ) {
        pipeline.advanceFrame(timeInMillis);

        for (uint16_t pixelIndex = 0; pixelIndex < segmentSize; ++pixelIndex) {
            auto [angle, radius] = context.polarCoordsMapper(pixelIndex);

            // Use the cached finalLayer. Since we know we only have one, we can
            // call the fast-path overload of blendLayers directly.
            segmentArray[pixelIndex] = blendLayers(
                angle,
                radius,
                timeInMillis,
                finalLayer
            );
        }
    }
}
