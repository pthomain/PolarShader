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

#include "VortexTransform.h"
#include "polar/pipeline/utils/MathUtils.h"

namespace LEDSegments {
    struct VortexTransform::State {
        ScalarMotion vortexSignal;

        explicit State(ScalarMotion vortex) : vortexSignal(std::move(vortex)) {
        }
    };

    VortexTransform::VortexTransform(ScalarMotion vortex)
        : state(std::make_shared<State>(std::move(vortex))) {
    }

    void VortexTransform::advanceFrame(Units::TimeMillis timeInMillis) {
        state->vortexSignal.advanceFrame(timeInMillis);
    }

    PolarLayer VortexTransform::operator()(const PolarLayer &layer) const {
        return [state = this->state, layer](Units::AngleTurnsUQ16_16 angle_q16, Units::FracQ0_16 radius) {
            // Get the vortex strength as a raw Q16.16 value.
            Units::RawFracQ16_16 vortex_strength_raw = state->vortexSignal.getRawValue();
            if (vortex_strength_raw < VORTEX_MIN.asRaw()) vortex_strength_raw = VORTEX_MIN.asRaw();
            if (vortex_strength_raw > VORTEX_MAX.asRaw()) vortex_strength_raw = VORTEX_MAX.asRaw();

            // Scale the vortex strength (Q16.16) by the radius (Q0.16) to get the angular offset.
            Units::RawFracQ16_16 offset_raw = scale_q16_16_by_f16(vortex_strength_raw, radius);

            // Add the full Q16.16 offset to the angle.
            Units::AngleTurnsUQ16_16 new_angle_q16 = angle_q16 + static_cast<uint32_t>(offset_raw);

            return layer(new_angle_q16, radius);
        };
    }
}
