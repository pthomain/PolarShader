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

#include "VortexTransform.h"
#include "renderer/pipeline/utils/MathUtils.h"

namespace PolarShader {
    struct VortexTransform::State {
        ScalarMotion vortexSignal;

        explicit State(ScalarMotion vortex) : vortexSignal(std::move(vortex)) {
        }
    };

    VortexTransform::VortexTransform(ScalarMotion vortex)
        : state(std::make_shared<State>(std::move(vortex))) {
    }

    void VortexTransform::advanceFrame(TimeMillis timeInMillis) {
        state->vortexSignal.advanceFrame(timeInMillis);
    }

    PolarLayer VortexTransform::operator()(const PolarLayer &layer) const {
        return [state = this->state, layer](AngleTurnsUQ16_16 angle_q16, RadiusQ0_16 radius) {
            // Get the vortex strength as a raw Q16.16 value.
            RawQ16_16 vortex_strength_raw = state->vortexSignal.getRawValue();
            if (raw(vortex_strength_raw) < VORTEX_MIN.asRaw()) vortex_strength_raw = RawQ16_16(VORTEX_MIN.asRaw());
            if (raw(vortex_strength_raw) > VORTEX_MAX.asRaw()) vortex_strength_raw = RawQ16_16(VORTEX_MAX.asRaw());

            // Scale the vortex strength (Q16.16) by the radius (Q0.16) to get the angular offset.
            RawQ16_16 offset_raw = RawQ16_16(
                scale_q16_16_by_f16(raw(vortex_strength_raw), toFracQ0_16(radius)));

            // Add the full Q16.16 offset to the angle.
            AngleTurnsUQ16_16 new_angle_q16 = wrapAddSigned(angle_q16, raw(offset_raw));

            return layer(new_angle_q16, radius);
        };
    }
}
