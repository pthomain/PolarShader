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

#ifndef LED_SEGMENTS_TRANSFORMS_TILEJITTERTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_TILEJITTERTRANSFORM_H

#include "base/Transforms.h"
#include "polar/pipeline/utils/Units.h"
#include "polar/pipeline/signals/motion/Motion.h"

namespace LEDSegments {
    /**
     * Adds a small, per-tile offset after tiling to break repetition. Uses tile index noise to jitter.
     *
     * Parameters: tileX, tileY (tile sizes), amplitude (ScalarMotion jitter magnitude in coordinate units).
     * Recommended order: immediately after TilingTransform in the Cartesian chain.
     */
    class TileJitterTransform : public CartesianTransform {
        uint32_t tileX;
        uint32_t tileY;
        ScalarMotion amplitudeSignal;

    public:
        TileJitterTransform(uint32_t tileX, uint32_t tileY, ScalarMotion amplitude);

        void advanceFrame(Units::TimeMillis timeInMillis) override;
        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_TILEJITTERTRANSFORM_H
