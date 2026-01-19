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

#include "RadialScaleTransform.h"

namespace PolarShader {
    struct RadialScaleTransform::State {
        ScalarMotion kSignal;

        explicit State(ScalarMotion k) : kSignal(std::move(k)) {
        }
    };

    RadialScaleTransform::RadialScaleTransform(ScalarMotion k)
        : state(std::make_shared<State>(std::move(k))) {
    }

    void RadialScaleTransform::advanceFrame(TimeMillis timeInMillis) {
        state->kSignal.advanceFrame(timeInMillis);
    }

    PolarLayer RadialScaleTransform::operator()(const PolarLayer &layer) const {
        return [state = this->state, layer](AngleTurnsUQ16_16 angle_q16, RadiusQ0_16 radius) {
            RawQ16_16 k_raw = state->kSignal.getRawValue();

            int64_t delta = (static_cast<int64_t>(raw(k_raw)) * static_cast<int64_t>(raw(radius))) >> 16;
            int64_t new_radius = static_cast<int64_t>(raw(radius)) + delta;

            if (new_radius < 0) new_radius = 0;
            if (new_radius > FRACT_Q0_16_MAX) new_radius = FRACT_Q0_16_MAX;

            RadiusQ0_16 r_out = RadiusQ0_16(static_cast<uint16_t>(new_radius));
            return layer(angle_q16, r_out);
        };
    }
}
