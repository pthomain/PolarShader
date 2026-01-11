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
     * @brief Applies a global rotation to the entire frame.
     * @param rotationSignal An AngularSignal providing the rotation in turns.
     */
    class RotationDecorator : public PolarDecorator {
        AngularSignal angle_turns;

    public:
        explicit RotationDecorator(AngularSignal rotationSignal) : angle_turns(std::move(rotationSignal)) {
        }

        void advanceFrame(unsigned long timeInMillis) override {
            angle_turns.advanceFrame(timeInMillis);
        }

        PolarLayer operator()(const PolarLayer &layer) const override {
            return [this, layer](uint16_t angle_turns, fract16 radius, unsigned long timeInMillis) {
                uint16_t new_angle_turns = angle_turns + this->angle_turns.getValue();
                return layer(new_angle_turns, radius, timeInMillis);
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_ROTATIONDECORATOR_H
