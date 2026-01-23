//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

/*
 * This file is part of PolarShader.
 *
 * PolarShader is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PolarShader is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PolarShader. If not, see <https://www.gnu.org/licenses/>.
 */

#include "RotationTransform.h"

namespace PolarShader {
    struct RotationTransform::State {
        BoundedAngularModulator rotationSignal;

        explicit State(BoundedAngularModulator rotation) : rotationSignal(std::move(rotation)) {
        }
    };

    RotationTransform::RotationTransform(BoundedAngularModulator rotation)
        : state(std::make_shared<State>(std::move(rotation))) {
    }

    void RotationTransform::advanceFrame(TimeMillis timeInMillis) {
        state->rotationSignal.advanceFrame(timeInMillis);
    }

    PolarLayer RotationTransform::operator()(const PolarLayer &layer) const {
        return [state = this->state, layer](UnboundedAngle angle_q16, BoundedScalar radius) {
            UnboundedAngle rotation = unbindAngle(state->rotationSignal.getPhase());
            UnboundedAngle new_angle_q16 = wrapAdd(angle_q16, raw(rotation));
            return layer(new_angle_q16, radius);
        };
    }
}
