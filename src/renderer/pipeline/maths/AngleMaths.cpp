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

#include "renderer/pipeline/maths/AngleMaths.h"
#include <FastLED.h>

namespace PolarShader {

    TrigQ1_15 angleSinQ1_15(BoundedAngle a) {
        return TrigQ1_15(sin16(angleToFastLedPhase(a)));
    }

    TrigQ1_15 angleSinQ1_15(UnboundedAngle p) {
        return TrigQ1_15(sin16(angleToFastLedPhase(p)));
    }

    TrigQ1_15 angleCosQ1_15(BoundedAngle a) {
        return TrigQ1_15(cos16(angleToFastLedPhase(a)));
    }

    TrigQ1_15 angleCosQ1_15(UnboundedAngle p) {
        return TrigQ1_15(cos16(angleToFastLedPhase(p)));
    }

    BoundedAngle angleAtan2TurnsApprox(int16_t y, int16_t x) {
        if (x == 0 && y == 0) return BoundedAngle(0);

        uint16_t abs_x = (x < 0) ? static_cast<uint16_t>(-x) : static_cast<uint16_t>(x);
        uint16_t abs_y = (y < 0) ? static_cast<uint16_t>(-y) : static_cast<uint16_t>(y);

        uint16_t max_val = (abs_x > abs_y) ? abs_x : abs_y;
        uint16_t min_val = (abs_x > abs_y) ? abs_y : abs_x;

        uint32_t z = (static_cast<uint32_t>(min_val) << 16) / max_val; // Q0.16
        uint32_t one_minus_z = Q16_16_ONE - z;

        static constexpr uint32_t A_Q16 = Q16_16_ONE / 8; // 0.125 turns in Q0.16
        static constexpr uint32_t B_Q16 = 2847u; // 0.04345 turns in Q0.16

        uint32_t inner = A_Q16 + ((B_Q16 * one_minus_z) >> 16);
        uint32_t base = (z * inner) >> 16; // 0..0.125 turns

        uint32_t angle = (abs_x >= abs_y) ? base : (QUARTER_TURN_U16 - base);
        if (x < 0) {
            angle = HALF_TURN_U16 - angle;
        }
        if (y < 0) {
            angle = Q16_16_ONE - angle;
        }

        return BoundedAngle(static_cast<uint16_t>(angle & ANGLE_U16_MAX));
    }
}
