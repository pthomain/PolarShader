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

#include "PolarSpec.h"
#include "config/PolarLayoutConfig.h"
#include "polar/pipeline/utils/MathUtils.h"

uint16_t PolarSpec::nbSegments(uint16_t layoutId) const {
    // The display can be treated as one whole segment or as 9 concentric rings.
    return layoutId == WHOLE ? 1 : 9;
}

uint16_t PolarSpec::segmentSize(uint16_t layoutId, uint16_t segmentIndex) const {
    // If treated as one segment, it contains all LEDs.
    if (layoutId == WHOLE) return 241;

    // Defines the number of LEDs in each concentric ring, from center outwards.
    switch (segmentIndex) {
        case 0: return 1; // Center
        case 1: return 8;
        case 2: return 12;
        case 3: return 16;
        case 4: return 24;
        case 5: return 32;
        case 6: return 40;
        case 7: return 48;
        case 8: return 60; // Outermost ring
        default: return 0;
    }
}

void PolarSpec::mapLeds(
    uint16_t layoutId,
    uint16_t segmentIndex,
    uint16_t pixelIndex,
    FracQ0_16 progress,
    const std::function<void(uint16_t)> &onLedMapped
) const {
    // Calculate the absolute LED index by summing the sizes of all previous segments.
    uint16_t offset = 0;
    for (uint16_t i = 0; i < segmentIndex; ++i) {
        offset += segmentSize(layoutId, i);
    }

    onLedMapped(offset + pixelIndex);
}

PolarCoords PolarSpec::toPolarCoords(uint16_t pixelIndex) const {
    uint16_t cumulativePixels = 0;
    const uint16_t numSegments = nbSegments(SEGMENTED);

    // Iterate through each ring to find which one the pixel belongs to.
    for (uint16_t segmentIndex = 0; segmentIndex < numSegments; ++segmentIndex) {
        const uint16_t currentSegmentSize = segmentSize(SEGMENTED, segmentIndex);

        if (pixelIndex < cumulativePixels + currentSegmentSize) {
            // The pixel is in this segment.
            const uint16_t pixelInSegment = pixelIndex - cumulativePixels;

            // The angle is the pixel's proportional position within its ring.
            // It's a uint16_t from 0 to 65535, representing 0 to 2PI radians.
            // For the center pixel (segment 0), the angle is 0.
            const AngleUnitsQ0_16 angleStep =
                    currentSegmentSize > 1 ? UINT16_MAX / currentSegmentSize : 0;
            AngleUnitsQ0_16 angle = pixelInSegment * angleStep;

            FracQ0_16 radius = divide_u16_as_fract16(
                segmentIndex,
                numSegments > 1 ? numSegments - 1 : 1
            );

            return {angle, radius};
        }

        cumulativePixels += currentSegmentSize;
    }

    // This should not be reached if pixelIndex is valid.
    return {0, 0};
}
