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

#ifndef LED_SEGMENTS_EFFECTS_DECORATORS_DECORATORS_LAYERS_H
#define LED_SEGMENTS_EFFECTS_DECORATORS_DECORATORS_LAYERS_H

#include "Layers.h"

namespace LEDSegments {
    /**
     * @class FrameDecorator
     * @brief The abstract base for all decorators.
     */
    class FrameDecorator {
    public:
        virtual ~FrameDecorator() = default;

        /**
         * @brief Evolves the decorator's internal state for the next frame.
         * @param timeInMillis The current animation time.
         * Decorators that have time-varying state should override this method.
         */
        virtual void advanceFrame(unsigned long timeInMillis) {
            // Default implementation is empty.
        }
    };

    /**
     * @class CartesianDecorator
     * @brief An interface for decorators that operate in the Cartesian (X, Y) domain.
     */
    class CartesianDecorator : public virtual FrameDecorator {
    public:
        virtual CartesianLayer operator()(const CartesianLayer &layer) const = 0;
    };

    /**
     * @class PolarDecorator
     * @brief An interface for decorators that operate in the Polar (Angle, Radius) domain.
     */
    class PolarDecorator : public virtual FrameDecorator {
    public:
        virtual PolarLayer operator()(const PolarLayer &layer) const = 0;
    };
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_DECORATORS_LAYERS_H
