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

#ifndef LED_SEGMENTS_TRANSFORMS_TILINGTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_TILINGTRANSFORM_H

#include "base/Transforms.h"

namespace LEDSegments {
    /**
     * Tiles Cartesian space by wrapping coordinates into a tile of size (tileX, tileY)
     * using signed modulo; negative coordinates wrap back into [0, tileN).
     * Useful for repeating patterns. A tile dimension of 0 leaves that axis unchanged.
     *
     * Parameters: tileX, tileY (unsigned tile sizes in coordinate units).
     * Recommended order: early in Cartesian chain; combine with shear/scale before polar conversion.
     */
    class TilingTransform : public CartesianTransform {
        uint32_t tileX;
        uint32_t tileY;

    public:
        TilingTransform(uint32_t tileX, uint32_t tileY);

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_TILINGTRANSFORM_H
