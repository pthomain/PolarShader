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

#ifndef LED_SEGMENTS_TRANSFORMS_SHEARTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_SHEARTRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "polar/pipeline/signals/motion/scalar/ScalarMotion.h"

namespace LEDSegments {
    /**
     * Applies a shear (skew) to Cartesian coordinates: x' = x + kx*y, y' = y + ky*x.
     * Shear coefficients are ScalarMotions (Q16.16), allowing animation. Wrap is explicit
     * modulo 2^32 to mirror other domain warps.
     *
     * Parameters: kx, ky (ScalarMotions, Q16.16).
     * Recommended order: early in the Cartesian chain, before other warps/tiling, so downstream transforms
     * see the skewed space.
     */
    class ShearTransform : public CartesianTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        ShearTransform(ScalarMotion kx, ScalarMotion ky);

        void advanceFrame(TimeMillis timeInMillis) override;

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_SHEARTRANSFORM_H
