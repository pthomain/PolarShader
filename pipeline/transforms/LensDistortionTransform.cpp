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

#include "LensDistortionTransform.h"

namespace LEDSegments {
    struct LensDistortionTransform::State {
        ScalarMotion kSignal;

        explicit State(ScalarMotion k) : kSignal(std::move(k)) {
        }
    };

    LensDistortionTransform::LensDistortionTransform(ScalarMotion k)
        : state(std::make_shared<State>(std::move(k))) {
    }

    void LensDistortionTransform::advanceFrame(Units::TimeMillis timeInMillis) {
        state->kSignal.advanceFrame(timeInMillis);
    }

    PolarLayer LensDistortionTransform::operator()(const PolarLayer &layer) const {
        return [state = this->state, layer](Units::AngleTurnsUQ16_16 angle_q16, Units::FracQ0_16 radius) {
            Units::RawFracQ16_16 k_raw = state->kSignal.getRawValue(); // Q16.16
            // factor = 1 + k * r
            int64_t factor_q16 = static_cast<int64_t>(Units::Q16_16_ONE) + ((static_cast<int64_t>(k_raw) * radius) >> 16);
            int64_t scaled = factor_q16 * radius; // Q16.16 * Q0.16 = Q16.32
            scaled = (scaled + (1LL << 15)) >> 16; // round to Q16.16
            if (scaled < 0) scaled = 0;
            if (scaled > Units::FRACT_Q0_16_MAX) scaled = Units::FRACT_Q0_16_MAX;
            Units::FracQ0_16 r_out = static_cast<Units::FracQ0_16>(scaled);
            return layer(angle_q16, r_out);
        };
    }
}
