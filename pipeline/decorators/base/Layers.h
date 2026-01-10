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

#ifndef LED_SEGMENTS_EFFECTS_DECORATORS_LAYERS_H
#define LED_SEGMENTS_EFFECTS_DECORATORS_LAYERS_H

#include "FastLED.h"

namespace LEDSegments {
    // A PolarLayer is a function that takes polar coordinates and returns a scalar value.
    using PolarLayer = fl::function<uint16_t(
            uint16_t angle,
            fract16 radius,
            unsigned long timeInMillis
        )
    >;

    // A CartesianLayer is a function that takes Cartesian coordinates and returns a scalar value, usually noise-based.
    using CartesianLayer = fl::function<uint16_t(
            uint32_t x,
            uint32_t y,
            unsigned long timeInMillis
        )
    >;

    // A ColourLayer is the final output of the pipeline, a function that returns a CRGB color.
    using ColourLayer = fl::function<CRGB(
            uint16_t angle,
            fract16 radius,
            unsigned long timeInMillis
        )
    >;
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_LAYERS_H
