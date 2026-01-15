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

#ifndef LED_SEGMENTS_TRANSFORMS_DOMAINWARPTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_DOMAINWARPTRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "polar/engine/pipeline/mappers/Signal.h"

namespace LEDSegments {
    /**
     * @class DomainWarpTransform
     * @brief Applies a smooth, low-frequency displacement to Cartesian coordinates.
     *
     * This transform distorts the input coordinates by adding a time-varying offset,
     * creating a "warping" or "flowing" effect in the final pattern.
     *
     * Input/output domain: CartesianLayer with 32-bit coordinates. Offsets are added with explicit
     * 32-bit wrap; no clamping occurs. Use when you want slow spatial drift before converting to polar.
     */
    class DomainWarpTransform : public CartesianTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        /**
         * @brief Constructs a new DomainWarpTransform.
         * @param xWarp A signal that provides the X component of the warp.
         * @param yWarp A signal that provides the Y component of the warp.
         */
        DomainWarpTransform(LinearSignal xWarp, LinearSignal yWarp);

        void advanceFrame(Units::TimeMillis timeInMillis) override;

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_DOMAINWARPTRANSFORM_H
