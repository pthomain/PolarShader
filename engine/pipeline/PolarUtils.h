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

    static fl::pair<int32_t, int32_t> cartesianCoords(
        fl::u16 angle_turns,
        fract16 radius
    ) {
        int32_t x = scale_i16_by_f16(cos16(angle_turns), radius);
        int32_t y = scale_i16_by_f16(sin16(angle_turns), radius);
        return {x, y};
    }

    static fl::pair<fl::u16, fract16> polarCoords(
        fl::i32 x,
        fl::i32 y
    ) {
        int32_t clampedX = constrain(x, INT16_MIN, INT16_MAX);
        int32_t clampedY = constrain(y, INT16_MIN, INT16_MAX);
        int16_t x16 = static_cast<int16_t>(clampedX);
        int16_t y16 = static_cast<int16_t>(clampedY);

        fl::u16 angle_turns = atan2_turns_approx(y16, x16);
        int32_t dx = x16;
        int32_t dy = y16;
        uint32_t radius_squared = static_cast<uint32_t>(dx * dx) + static_cast<uint32_t>(dy * dy);
        uint16_t magnitude = sqrt_u32(radius_squared);
        uint32_t radius_q16 = (static_cast<uint32_t>(magnitude) << 16) / 32767u;
        if (radius_q16 > UINT16_MAX) radius_q16 = UINT16_MAX;
        fract16 radius = static_cast<fract16>(radius_q16);
        return {angle_turns, radius};
    }
}
#endif //LED_SEGMENTS_SPECS_POLARUTILS_H
