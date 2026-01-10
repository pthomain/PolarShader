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

#ifndef LED_SEGMENTS_EFFECTS_DECORATORS_ROTATIONDECORATOR_H
#define LED_SEGMENTS_EFFECTS_DECORATORS_ROTATIONDECORATOR_H

#include "base/Decorators.h"
#include "../mappers/ValueMappers.h"

namespace LEDSegments {
    /**
     * @class RotationDecorator
     * @brief A stateful Polar decorator that applies a global angular rotation to the entire frame.
     *
     * The rotation speed is controlled by a ValueMapper, creating the potential for
     * organic, wandering motion. It operates on the 16-bit angular domain.
     */
    class RotationDecorator : public PolarDecorator {
        uint16_t globalAngle = 0;
        DeltaValue angleMapper;

    public:
        explicit RotationDecorator(DeltaValue velocityMapper) : angleMapper(std::move(velocityMapper)) {
        }

        /**
         * @brief Evolves the global rotation angle for the next frame.
         * The angular velocity is determined by the mapper and then integrated
         * into the current angle with smoothing.
         */
        void advanceFrame(unsigned long timeInMillis) override {
            globalAngle += angleMapper(timeInMillis);
        }

        PolarLayer operator()(const PolarLayer &layer) const override {
            // Capture 'this' to access the current globalAngle at render time.
            return [this, layer](uint16_t angle, fract16 radius, unsigned long timeInMillis) {
                // Apply the global rotation. The uint16_t arithmetic correctly
                // wraps around within the 0-65535 angular domain.
                uint16_t newAngle = angle + globalAngle;
                return layer(newAngle, radius, timeInMillis);
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_ROTATIONDECORATOR_H
