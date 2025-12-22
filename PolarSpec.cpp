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

uint16_t PolarSpec::nbSegments(uint16_t layoutId) const {
    return layoutId == WHOLE ? 1 : 9;
}

uint16_t PolarSpec::segmentSize(uint16_t layoutId, uint16_t segmentIndex) const {
    if (layoutId == WHOLE) return 241;

    switch (segmentIndex) {
        case 0: return 1;
        case 1: return 8;
        case 2: return 12;
        case 3: return 16;
        case 4: return 24;
        case 5: return 32;
        case 6: return 40;
        case 7: return 48;
        case 8: return 60;
        default: return 0;
    }
}

void PolarSpec::mapLeds(
    uint16_t layoutId,
    uint16_t segmentIndex,
    uint16_t pixelIndex,
    fract16 progress,
    const std::function<void(uint16_t)> &onLedMapped
) const {
    uint16_t offset = 0;

    for (uint16_t i = 0; i < segmentIndex; ++i) {
        offset += segmentSize(layoutId, i);
    }

    onLedMapped(offset + pixelIndex);
}
