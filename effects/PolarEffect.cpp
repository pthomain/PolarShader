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

#include "polar/pipeline/utils/PolarUtils.h"
#include "PolarEffect.h"
#include "polar/pipeline/presets/PresetPicker.h"
#include "polar/pipeline/presets/Presets.h"

namespace LEDSegments {
    static const PolarEffectFactory factoryInstance;
    RenderableFactoryRef<CRGB> PolarEffect::factory = &factoryInstance;

    PolarEffect::PolarEffect(
        const RenderableContext &context
    ) : Effect(context),
        pipeline(buildDefaultPreset(context.palette.palette)) {
        //PresetPicker::pickRandom(context.palette.palette)) {
        colourLayer = pipeline.build();
    }

    CRGB PolarEffect::blendLayers(
        Units::PhaseTurnsUQ16_16 angle_q16,
        Units::FractQ0_16 radius,
        const ColourLayer &layer
    ) {
        return layer(angle_q16, radius);
    }

    CRGB PolarEffect::blendLayers(
        Units::PhaseTurnsUQ16_16 angle_q16,
        Units::FractQ0_16 radius,
        const fl::vector<ColourLayer> &layers
    ) {
        if (layers.empty()) return CRGB::Black;
        if (layers.size() == 1) {
            return layers[0](angle_q16, radius);
        }
        CRGB16 blended16;
        for (const auto &layer: layers) {
            blended16 += layer(angle_q16, radius);
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
        Units::FractQ0_16 progress,
        Units::TimeMillis timeInMillis
    ) {
        pipeline.advanceFrame(timeInMillis);
        for (uint16_t pixelIndex = 0; pixelIndex < segmentSize; ++pixelIndex) {
            auto [angle_turns, radius] = context.polarCoordsMapper(pixelIndex);
            Units::PhaseTurnsUQ16_16 angle_q16 = Units::angleTurnsToPhaseQ16_16(angle_turns);
            segmentArray[pixelIndex] = blendLayers(
                angle_q16,
                radius,
                colourLayer
            );
        }
    }
}
