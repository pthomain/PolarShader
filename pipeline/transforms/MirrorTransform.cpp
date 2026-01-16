//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

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

#include "MirrorTransform.h"
#include <cstdint>

namespace LEDSegments {

    MirrorTransform::MirrorTransform(bool mirrorX, bool mirrorY)
        : mirrorX(mirrorX), mirrorY(mirrorY) {}

    CartesianLayer MirrorTransform::operator()(const CartesianLayer &layer) const {
        return [mirrorX = this->mirrorX, mirrorY = this->mirrorY, layer](int32_t x, int32_t y) {
            auto abs_i32 = [](int32_t v) -> int32_t {
                if (v == INT32_MIN) return INT32_MAX; // saturate to avoid UB on negation
                return v < 0 ? -v : v;
            };
            int32_t mx = mirrorX && x < 0 ? abs_i32(x) : x;
            int32_t my = mirrorY && y < 0 ? abs_i32(y) : y;
            return layer(mx, my);
        };
    }
}
