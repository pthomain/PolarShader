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

#ifndef LED_SEGMENTS_TRANSFORMS_PERSPECTIVEWARPTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_PERSPECTIVEWARPTRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "polar/pipeline/signals/motion/Motion.h"

namespace LEDSegments {
    /**
     * Applies a simple perspective warp: scales coordinates by a factor based on Y (vanishing point).
     * x' = x * scale, y' = y * scale where scale = 1 / (1 + k * y). Clamp to avoid divide-by-zero.
     *
     * Parameters: k (ScalarMotion, Q16.16). Positive k pulls far Y inward; negative pushes outward.
     * Recommended order: after base Cartesian warps (shear/scale), before tiling/mirroring or polar conversion.
     */
    class PerspectiveWarpTransform : public CartesianTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        explicit PerspectiveWarpTransform(ScalarMotion k);

        void advanceFrame(Units::TimeMillis timeInMillis) override;

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_PERSPECTIVEWARPTRANSFORM_H
