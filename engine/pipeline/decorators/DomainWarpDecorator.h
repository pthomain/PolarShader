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

#ifndef LED_SEGMENTS_EFFECTS_DECORATORS_DOMAINWARPDECORATOR_H
#define LED_SEGMENTS_EFFECTS_DECORATORS_DOMAINWARPDECORATOR_H

#include "base/Decorators.h"
#include "polar/engine/camera/CameraRig.h"

namespace LEDSegments {
    /**
     * @class DomainWarpDecorator
     * @brief Applies a smooth, low-frequency displacement to Cartesian coordinates before noise sampling.
     *
     * Units: Accumulates integer-domain deltas from CameraRig; no zoom scaling applied.
     * IMPORTANT CONSTRAINTS
     * - Does not scale by zoom
     * - Does not sample noise
     * - Uses only velocity-driven deltas provided by CameraRig
     */
    class DomainWarpDecorator : public CartesianDecorator {
        CameraRig &camera;

        int32_t accumX = 0;
        int32_t accumY = 0;

    public:
        explicit DomainWarpDecorator(CameraRig &rig) : camera(rig) {}

        void advanceFrame(unsigned long /*t*/) override {
            // CameraRig is the single source of temporal truth.
            // We only accumulate the already-integrated per-frame deltas it provides.
            accumX += camera.warpDeltaX();
            accumY += camera.warpDeltaY();
        }

        CartesianLayer operator()(const CartesianLayer &layer) const override {
            return [this, layer](int32_t x, int32_t y, unsigned long t) {
                return layer(x + accumX, y + accumY, t);
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_DOMAINWARPDECORATOR_H
