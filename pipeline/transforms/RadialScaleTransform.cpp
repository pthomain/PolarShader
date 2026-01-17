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

#include "RadialScaleTransform.h"

namespace LEDSegments {
    struct RadialScaleTransform::State {
        ScalarMotion kSignal;

        explicit State(ScalarMotion k) : kSignal(std::move(k)) {
        }
    };

    RadialScaleTransform::RadialScaleTransform(ScalarMotion k)
        : state(std::make_shared<State>(std::move(k))) {
    }

    void RadialScaleTransform::advanceFrame(Units::TimeMillis timeInMillis) {
        state->kSignal.advanceFrame(timeInMillis);
    }

    PolarLayer RadialScaleTransform::operator()(const PolarLayer &layer) const {
        return [state = this->state, layer](Units::AngleTurnsUQ16_16 angle_q16, Units::FracQ0_16 radius) {
            Units::RawFracQ16_16 k_raw = state->kSignal.getRawValue();

            int64_t delta = (static_cast<int64_t>(k_raw) * static_cast<int64_t>(radius)) >> 16;
            int64_t new_radius = static_cast<int64_t>(radius) + delta;

            if (new_radius < 0) new_radius = 0;
            if (new_radius > Units::FRACT_Q0_16_MAX) new_radius = Units::FRACT_Q0_16_MAX;

            Units::FracQ0_16 r_out = static_cast<Units::FracQ0_16>(new_radius);
            return layer(angle_q16, r_out);
        };
    }
}
