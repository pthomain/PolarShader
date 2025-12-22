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
        uint16_t radius = 0;
        uint16_t cumulativePixels = 0;

        for (uint16_t pixelIndex = 0; pixelIndex < segmentSize; pixelIndex++) {
            uint16_t actualSegmentSize = 0;

            switch (radius) {
                case 0: actualSegmentSize = 1;
                    break;
                case 1: actualSegmentSize = 8;
                    break;
                case 2: actualSegmentSize = 12;
                    break;
                case 3: actualSegmentSize = 16;
                    break;
                case 4: actualSegmentSize = 24;
                    break;
                case 5: actualSegmentSize = 32;
                    break;
                case 6: actualSegmentSize = 40;
                    break;
                case 7: actualSegmentSize = 48;
                    break;
                case 8: actualSegmentSize = 60;
                    break;
                default: actualSegmentSize = 0;
                    break;
            }

            if (pixelIndex > cumulativePixels + actualSegmentSize) {
                radius++;
                cumulativePixels += actualSegmentSize;
            }

            uint8_t angleUnit = actualSegmentSize == 0 ? 0 : map16_to_8(max(1, 65535 / actualSegmentSize));
            uint8_t angle = (pixelIndex - cumulativePixels) * angleUnit;

            uint8_t x = cos8(angle) * radius;
            uint8_t y = sin8(angle) * radius;
            const uint16_t noise = normaliseNoise(inoise8(x, y, timeInMillis >> 3));

            segmentArray[pixelIndex] = CHSV(noise, 255, 255);
        }
    }
}
