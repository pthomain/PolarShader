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

#include "FastLED.h"
#include "PolarEffect.h"
#include "engine/utils/Weights.h"

namespace LEDSegments {
    static const PolarEffectFactory factoryInstance;
    RenderableFactoryRef<CRGB> PolarEffect::factory = &factoryInstance;

    void fillPolarNoise(
        CRGB *segmentArray,
        uint16_t pixelIndex,
        fract16 angle,
        fract16 radius,
        unsigned long timeInMillis
    ) {
        uint16_t x = qmul16u(cos16(angle), radius);
        uint16_t y = qmul16u(sin16(angle), radius);
        const uint8_t noise = map16_to_8(inoise16(x, y, timeInMillis << 5));
        segmentArray[pixelIndex] = CHSV(
            normaliseNoise(noise),
            255,
            255
        );
    }

    void PolarEffect::fillSegmentArray(
        CRGB *segmentArray,
        uint16_t segmentSize,
        uint16_t segmentIndex,
        fract16 progress,
        unsigned long timeInMillis
    ) {
        polarContext.reset();
        utils.time16 += 64;
        utils.rotate16 += 32;

        for (uint16_t pixelIndex = 0; pixelIndex < segmentSize; ++pixelIndex) {
            context.polarCoords(pixelIndex, polarContext);

            utils.fillPolar(
            // fillPolarNoise(
                segmentArray,
                pixelIndex,
                polarContext.angle,
                polarContext.radius,
                timeInMillis
            );
        }
    }
}
