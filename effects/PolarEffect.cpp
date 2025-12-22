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
#include "engine/utils/Weights.h"

namespace LEDSegments {
    static const PolarEffectFactory factoryInstance;
    RenderableFactoryRef<CRGB> PolarEffect::factory = &factoryInstance;

    void PolarEffect::fillSegmentArray(
        CRGB *segmentArray,
        uint16_t segmentSize,
        uint16_t segmentIndex,
        fract16 progress,
        unsigned long timeInMillis
    ) {
        //TODO move to effect context
        polarContext.reset();

        //TODO move to base
        auto polarCoords = context.polarCoords;
        if (polarCoords == nullptr) {
            Serial.println("PolarEffect::fillSegmentArray: polarCoords is nullptr");
            return;
        }

        for (int pixelIndex = 0; pixelIndex < segmentSize; ++pixelIndex) {
            polarCoords->operator()(pixelIndex, polarContext);

            uint8_t x = cos8(polarContext.angle) * polarContext.radius;
            uint8_t y = sin8(polarContext.angle) * polarContext.radius;
            const uint16_t noise = normaliseNoise(inoise8(x, y, timeInMillis >> 3));

            segmentArray[pixelIndex] = CHSV(noise, 255, 255);
        }
    }
}
