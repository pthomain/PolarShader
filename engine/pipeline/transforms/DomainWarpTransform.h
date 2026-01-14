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

#ifndef LED_SEGMENTS_EFFECTS_TRANSFORMS_DOMAINWARPTRANSFORM_H
#define LED_SEGMENTS_EFFECTS_TRANSFORMS_DOMAINWARPTRANSFORM_H

#include "base/Transforms.h"
#include "polar/engine/pipeline/mappers/Signal.h"

namespace LEDSegments {
    /**
     * @class DomainWarpTransform
     * @brief Applies a smooth, low-frequency displacement to Cartesian coordinates.
     *
     * This transform distorts the input coordinates by adding a time-varying offset,
     * creating a "warping" or "flowing" effect in the final pattern.
     */
    class DomainWarpTransform : public CartesianTransform {
        LinearSignal warpXSignal;
        LinearSignal warpYSignal;

        int32_t lastWarpPosX_int = 0;
        int32_t lastWarpPosY_int = 0;
        int32_t accumX = 0;
        int32_t accumY = 0;

    public:
        /**
         * @brief Constructs a new DomainWarpTransform.
         * @param xWarp A signal that provides the X component of the warp.
         * @param yWarp A signal that provides the Y component of the warp.
         */
        DomainWarpTransform(LinearSignal xWarp, LinearSignal yWarp)
            : warpXSignal(std::move(xWarp)), warpYSignal(std::move(yWarp)) {
            lastWarpPosX_int = warpXSignal.getValue();
            lastWarpPosY_int = warpYSignal.getValue();
        }

        void advanceFrame(unsigned long timeInMillis) override {
            warpXSignal.advanceFrame(timeInMillis);
            warpYSignal.advanceFrame(timeInMillis);

            int32_t currentWarpPosX = warpXSignal.getValue();
            int32_t currentWarpPosY = warpYSignal.getValue();

            accumX += currentWarpPosX - lastWarpPosX_int;
            accumY += currentWarpPosY - lastWarpPosY_int;

            lastWarpPosX_int = currentWarpPosX;
            lastWarpPosY_int = currentWarpPosY;
        }

        CartesianLayer operator()(const CartesianLayer &layer) const override {
            return [this, layer](int32_t x, int32_t y, unsigned long t) {
                return layer(x + accumX, y + accumY, t);
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_TRANSFORMS_DOMAINWARPTRANSFORM_H
