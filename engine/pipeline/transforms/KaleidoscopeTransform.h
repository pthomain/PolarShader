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

#ifndef LED_SEGMENTS_EFFECTS_TRANSFORMS_KALEIDOSCOPETRANSFORM_H
#define LED_SEGMENTS_EFFECTS_TRANSFORMS_KALEIDOSCOPETRANSFORM_H

#include "base/Transforms.h"

namespace LEDSegments {
    /**
     * @class KaleidoscopeTransform
     * @brief A stateless Polar transform that folds the angular space to create symmetry.
     *
     * The number of segments is controlled by a ValueMapper, which provides a
     * normalized 0-65535 value that this transform maps to the segment count.
     */
    class KaleidoscopeTransform : public PolarTransform {
        uint8_t nbSegments;
        bool isMandala;
        bool isMirroring;

        static constexpr uint8_t MAX_SEGMENTS = 8;

        static uint16_t foldAngleKaleidoscope(
            fl::u16 angle,
            fl::u8 nbSegments,
            bool isMirroring
        ) {
            if (nbSegments <= 1) return angle;

            uint16_t segmentAngle = 0x10000 / nbSegments;
            uint16_t foldedAngle = angle % segmentAngle;

            bool isOdd = (angle / segmentAngle) & 1;
            if (isMirroring && isOdd) return segmentAngle - foldedAngle;

            return foldedAngle;
        }

    public:
        explicit KaleidoscopeTransform(
            uint8_t nbSegments,
            bool isMandala = false,
            bool isMirroring = true
        ) : nbSegments(nbSegments), isMandala(isMandala), isMirroring(isMirroring) {
        }

        PolarLayer operator()(const PolarLayer &layer) const override {
            return [this, layer](uint16_t angle, fract16 radius, unsigned long timeInMillis) {
                uint8_t segments = constrain(nbSegments, (uint8_t)1, MAX_SEGMENTS);

                uint16_t newAngle = isMandala
                                        ? angle * segments
                                        : foldAngleKaleidoscope(angle, segments, isMirroring);

                return layer(newAngle, radius, timeInMillis);
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_TRANSFORMS_KALEIDOSCOPETRANSFORM_H
