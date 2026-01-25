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
        FracQ0_16 angle_turns,
        FracQ0_16 radius
    ) {
        TrigQ0_16 cos_val = angleCosQ0_16(angle_turns);
        TrigQ0_16 sin_val = angleSinQ0_16(angle_turns);
        int32_t x = scale32(raw(cos_val), radius);
        int32_t y = scale32(raw(sin_val), radius);
        return {x, y};
    }

    fl::pair<FracQ0_16, FracQ0_16> cartesianToPolar(fl::i32 x, fl::i32 y) {
        int32_t clamped_x = constrain(x, Q0_16_MIN, Q0_16_MAX);
        int32_t clamped_y = constrain(y, Q0_16_MIN, Q0_16_MAX);
        int16_t x16 = static_cast<int16_t>(clamped_x >> 1);
        int16_t y16 = static_cast<int16_t>(clamped_y >> 1);

        FracQ0_16 angle_units = angleAtan2TurnsApprox(y16, x16);
        int64_t dx = clamped_x;
        int64_t dy = clamped_y;

        uint64_t radius_squared = static_cast<uint64_t>(dx * dx) + static_cast<uint64_t>(dy * dy);
        uint64_t magnitude_raw = sqrtU64(radius_squared);

        if (magnitude_raw > static_cast<uint64_t>(FRACT_Q0_16_MAX)) {
            magnitude_raw = FRACT_Q0_16_MAX;
        }

        FracQ0_16 radius = FracQ0_16(static_cast<uint16_t>(magnitude_raw));
        return {angle_units, radius};
    }
}
