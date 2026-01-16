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

#include "RotationTransform.h"

namespace LEDSegments {
    struct RotationTransform::State {
        AngularSignal rotationSignal;

        explicit State(AngularSignal rotation) : rotationSignal(std::move(rotation)) {
        }
    };

    RotationTransform::RotationTransform(AngularSignal rotation)
        : state(std::make_shared<State>(std::move(rotation))) {
    }

    void RotationTransform::advanceFrame(Units::TimeMillis timeInMillis) {
        state->rotationSignal.advanceFrame(timeInMillis);
    }

    PolarLayer RotationTransform::operator()(const PolarLayer &layer) const {
        return [state = this->state, layer](Units::PhaseTurnsUQ16_16 angle_q16, Units::FractQ0_16 radius) {
            // Add the full Q16.16 rotation value to the angle
            uint32_t delta = static_cast<uint32_t>(state->rotationSignal.getRawValue());
            Units::PhaseTurnsUQ16_16 new_angle_q16 = angle_q16 + delta;
            return layer(new_angle_q16, radius);
        };
    }
}
