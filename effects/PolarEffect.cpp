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
#include "polar/engine/pipeline/CartesianNoiseLayers.h"
#include "polar/engine/pipeline/transforms/KaleidoscopeTransform.h"
#include "polar/engine/pipeline/transforms/DomainWarpTransform.h"
#include "polar/engine/pipeline/transforms/VortexTransform.h"
#include "polar/engine/pipeline/transforms/RotationTransform.h"

namespace LEDSegments {
    static const PolarEffectFactory factoryInstance;
    RenderableFactoryRef<CRGB> PolarEffect::factory = &factoryInstance;

    PolarEffect::PolarEffect(
        const RenderableContext &context
    ) : Effect(context),
        pipeline(
            PolarPipelineBuilder(std::move(noiseLayer), context.palette.palette)
            .addCartesianTransform(
                DomainWarpTransform(
                    LinearSignal(
                        0,
                        Waveforms::Noise(
                            Waveforms::ConstantWaveform(100),
                            Waveforms::ConstantWaveform(1000)
                        )
                    ),
                    LinearSignal(
                        0,
                        Waveforms::Noise(
                            Waveforms::ConstantWaveform(100),
                            Waveforms::ConstantWaveform(1000)
                        )
                    )
                )
            )
            .addPolarTransform(
                RotationTransform(
                    AngularSignal(
                        0,
                        Waveforms::Constant(500)
                    )
                )
            )
            .addPolarTransform(
                VortexTransform(
                    BoundedSignal(
                        0,
                        Waveforms::Pulse(
                            Waveforms::ConstantWaveform(200),
                            Waveforms::ConstantWaveform(20000)
                        ),
                        500,
                        -30000,
                        30000
                    )
                )
            )
            .addPolarTransform(
                KaleidoscopeTransform(
                    3,
                    false,
                    true
                )
            )
            .build()
        ) {
        colourLayer = pipeline.build();
    }

    CRGB PolarEffect::blendLayers(
        uint16_t angle_turns,
        fract16 radius,
        unsigned long timeInMillis,
        const ColourLayer &layer
    ) {
        return layer(angle_turns, radius, timeInMillis);
    }

    CRGB PolarEffect::blendLayers(
        uint16_t angle_turns,
        fract16 radius,
        unsigned long timeInMillis,
        const fl::vector<ColourLayer> &layers
    ) {
        if (layers.empty()) return CRGB::Black;
        if (layers.size() == 1) {
            return layers[0](angle_turns, radius, timeInMillis);
        }
        CRGB16 blended16;
        for (const auto &layer: layers) {
            blended16 += layer(angle_turns, radius, timeInMillis);
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
            auto [angle_turns, radius] = context.polarCoordsMapper(pixelIndex);
            segmentArray[pixelIndex] = blendLayers(
                angle_turns,
                radius,
                timeInMillis,
                colourLayer
            );
        }
    }
}
