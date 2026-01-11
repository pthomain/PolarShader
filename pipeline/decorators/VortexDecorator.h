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

#ifndef LED_SEGMENTS_EFFECTS_DECORATORS_VORTEXDECORATOR_H
#define LED_SEGMENTS_EFFECTS_DECORATORS_VORTEXDECORATOR_H

#include "base/Decorators.h"
#include "../mappers/ValueMappers.h"

namespace LEDSegments {
    /**
     * @class VortexDecorator
     * @brief Applies a radius-dependent angular offset (twist) to the polar coordinates.
     *
     * @param vortexSignal A BoundedSignal providing the strength of the vortex in turns.
     *                     The signal's value is interpreted as the total angular twist
     *                     applied at the maximum radius (radius = 1.0).
     *                     For example, a value of 32768 corresponds to a 0.5 turn twist.
     */
    class VortexDecorator : public PolarDecorator {
        BoundedSignal vortex_strength_turns;

    public:
        explicit VortexDecorator(BoundedSignal vortexSignal)
            : vortex_strength_turns(std::move(vortexSignal)) {
        }

        void advanceFrame(unsigned long timeInMillis) override {
            vortex_strength_turns.advanceFrame(timeInMillis);
        }

        PolarLayer operator()(const PolarLayer &layer) const override {
            return [this, layer](uint16_t angle_turns, fract16 radius, unsigned long timeInMillis) {
                // The signal value is a signed int, representing the vortex strength in turns.
                // We constrain it to a 16-bit signed range before scaling.
                int16_t strength_turns = constrain(
                    vortex_strength_turns.getValue(),
                    INT16_MIN,
                    INT16_MAX
                );

                // Scale the twist by the current pixel's radius using signed math.
                int16_t offset_turns = scale_i16_by_f16(strength_turns, radius);
                uint16_t new_angle_turns = angle_turns + offset_turns;
                return layer(new_angle_turns, radius, timeInMillis);
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_VORTEXDECORATOR_H
