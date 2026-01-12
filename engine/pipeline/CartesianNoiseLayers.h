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

#ifndef LED_SEGMENTS_SPECS_POLARLAYERS_H
#define LED_SEGMENTS_SPECS_POLARLAYERS_H

#include "utils/MathUtils.h"

namespace LEDSegments {
    inline uint16_t noiseLayer(
        uint32_t x,
        uint32_t y,
        unsigned long /*timeInMillis*/
    ) {
        return inoise16(x, y);
    }

    inline uint16_t fBmLayer(
        uint32_t x,
        uint32_t y,
        unsigned long /*timeInMillis*/
    ) {
        static fl::u8 octaves = 4;
        uint16_t r = 0;
        uint16_t amplitude = U16_HALF;
        for (int o = 0; o < octaves; o++) {
            r += (inoise16(x, y) * amplitude) >> 16;
            x <<= 1;
            y <<= 1;
            amplitude >>= 1;
        }
        return r;
    }

    inline uint16_t turbulenceLayer(
        uint32_t x,
        uint32_t y,
        unsigned long /*timeInMillis*/
    ) {
        int16_t r = (int16_t) inoise16(x, y) - 32768;
        uint16_t mag = (uint16_t) (r ^ (r >> 15)) - (uint16_t) (r >> 15);
        return mag << 1;
    }

    inline uint16_t ridgedLayer(
        uint32_t x,
        uint32_t y,
        unsigned long /*timeInMillis*/
    ) {
        int16_t r = (int16_t) inoise16(x, y) - 32768;
        uint16_t mag = (uint16_t) (r ^ (r >> 15)) - (uint16_t) (r >> 15);
        mag = min<uint16_t>(mag, 32767);
        uint16_t inverted = 65535 - (mag << 1);
        return inverted;
    }
}
#endif //LED_SEGMENTS_SPECS_POLARLAYERS_H

