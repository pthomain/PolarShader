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

#ifndef LED_SEGMENTS_EFFECTS_TRANSFORMS_VIEWPORTTRANSFORM_H
#define LED_SEGMENTS_EFFECTS_TRANSFORMS_VIEWPORTTRANSFORM_H

#include "base/Transforms.h"
#include "../ViewPort.h"

namespace LEDSegments {
    /**
     * @class ViewPortTransform
     * @brief A thin adapter that applies camera transformations to the Cartesian coordinate space.
     */
    class ViewPortTransform : public CartesianTransform {
        ViewPort &viewPort;

    public:
        explicit ViewPortTransform(ViewPort &viewPort) : viewPort(viewPort) {
        }

        void advanceFrame(unsigned long t) override {
            // The camera rig is the single source of truth for advancing time.
            viewPort.advanceFrame(t);
        }

        CartesianLayer operator()(const CartesianLayer &layer) const override {
            return [this, layer](int32_t x, int32_t y, unsigned long t) {
                // Apply the inverse scale to "zoom" the coordinate space.
                int32_t sx = scale_i32_by_q16_16(x, viewPort.inverseLinearScale());
                int32_t sy = scale_i32_by_q16_16(y, viewPort.inverseLinearScale());

                // Apply translation.
                return layer(
                    sx + viewPort.positionX(),
                    sy + viewPort.positionY(),
                    t
                );
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_TRANSFORMS_VIEWPORTTRANSFORM_H
