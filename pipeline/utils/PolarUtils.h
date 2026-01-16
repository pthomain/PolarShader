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

#ifndef LED_SEGMENTS_PIPELINE_POLARUTILS_H
#define LED_SEGMENTS_PIPELINE_POLARUTILS_H

#include "Units.h"

namespace LEDSegments {
    struct CRGB16 {
        uint16_t r;
        uint16_t g;
        uint16_t b;

        CRGB16(uint16_t r = 0, uint16_t g = 0, uint16_t b = 0);

        CRGB16 &operator+=(const CRGB &rhs);
    };

    // Phase stores AngleTurns16 in the high 16 bits; trig sampling uses (phase >> 16). Callers must
    // pass a promoted PhaseTurnsUQ16_16; no auto-promotion happens here.
    fl::pair<int32_t, int32_t> cartesianCoords(
        Units::PhaseTurnsUQ16_16 angle_q16,
        Units::FractQ0_16 radius
    );

    fl::pair<Units::PhaseTurnsUQ16_16, Units::FractQ0_16> polarCoords(
        fl::i32 x,
        fl::i32 y
    );
}
#endif //LED_SEGMENTS_PIPELINE_POLARUTILS_H
