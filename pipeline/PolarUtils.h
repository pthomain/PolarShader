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

#ifndef LED_SEGMENTS_SPECS_POLARUTILS_H
#define LED_SEGMENTS_SPECS_POLARUTILS_H

#include "utils/MathUtils.h"

namespace LEDSegments {
    struct CRGB16 {
        uint16_t r;
        uint16_t g;
        uint16_t b;

        CRGB16(uint16_t r = 0, uint16_t g = 0, uint16_t b = 0) : r(r), g(g), b(b) {
        }

        CRGB16 &operator+=(const CRGB &rhs) {
            r += rhs.r;
            g += rhs.g;
            b += rhs.b;
            return *this;
        }
    };

    /**
     * @brief Converts polar coordinates to a clean, signed Cartesian space.
     * @param angle_turns The polar angle in turns (1.0 turn = 65536 units).
     * @param radius The polar radius (fract16 from 0-65535, representing 0.0 to ~1.0).
     * @return A pair of {x, y} coordinates in a signed 32-bit integer space.
     */
    static fl::pair<int32_t, int32_t> cartesianCoords(
        fl::u16 angle_turns,
        fract16 radius
    ) {
        // Convert polar to signed cartesian space at full resolution
        int32_t x = scale_i16_by_f16(cos16(angle_turns), radius);
        int32_t y = scale_i16_by_f16(sin16(angle_turns), radius);
        return {x, y};
    }
}
#endif //LED_SEGMENTS_SPECS_POLARUTILS_H
