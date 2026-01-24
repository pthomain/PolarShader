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

#include "renderer/pipeline/maths/PolarMaths.h"
#include <Arduino.h>
#include "renderer/pipeline/maths/AngleMaths.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include "renderer/pipeline/units/UnitConstants.h"

namespace PolarShader {
    fl::pair<int32_t, int32_t> polarToCartesian(
        UnboundedAngle angle_phase,
        BoundedScalar radius
    ) {
        // FastLED trig expects a 16-bit angle sample where 65536 == one turn. Phase must have been promoted
        // (AngleUnitsQ0_16 << 16); otherwise the angle collapses to zero and artefacts appear.
        BoundedAngle angle_turns = phaseToAngle(angle_phase);
        TrigQ1_15 cos_val = angleCosQ1_15(angle_turns);
        TrigQ1_15 sin_val = angleSinQ1_15(angle_turns);
        int32_t x = scalarScaleByBounded(raw(cos_val), radius);
        int32_t y = scalarScaleByBounded(raw(sin_val), radius);
        return {x, y};
    }

    fl::pair<UnboundedAngle, BoundedScalar> cartesianToPolar(fl::i32 x, fl::i32 y) {
        int32_t clamped_x = constrain(x, (int32_t)INT16_MIN, (int32_t)INT16_MAX);
        int32_t clamped_y = constrain(y, (int32_t)INT16_MIN, (int32_t)INT16_MAX);
        int16_t x16 = static_cast<int16_t>(clamped_x);
        int16_t y16 = static_cast<int16_t>(clamped_y);

        BoundedAngle angle_units = angleAtan2TurnsApprox(y16, x16);
        // Promote the 16-bit sample to phase (angle units in high 16 bits).
        UnboundedAngle angle_phase = angleToPhase(angle_units);

        int32_t dx = x16;
        int32_t dy = y16;
        uint32_t radius_squared = static_cast<uint32_t>(dx * dx) + static_cast<uint32_t>(dy * dy);
        uint16_t magnitude = scalarSqrtU32(radius_squared);
        uint32_t radius_q16 = (static_cast<uint32_t>(magnitude) << 16) / static_cast<uint32_t>(TRIG_Q1_15_MAX);
        if (radius_q16 > UINT16_MAX) radius_q16 = UINT16_MAX;
        BoundedScalar radius = BoundedScalar(static_cast<uint16_t>(radius_q16));
        return {angle_phase, radius};
    }
}
