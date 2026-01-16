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

#include "PolarUtils.h"
#include <Arduino.h>
#include "MathUtils.h"

namespace LEDSegments {
    CRGB16::CRGB16(uint16_t r, uint16_t g, uint16_t b) : r(r), g(g), b(b) {
    }

    CRGB16 &CRGB16::operator+=(const CRGB &rhs) {
        r += rhs.r;
        g += rhs.g;
        b += rhs.b;
        return *this;
    }

    fl::pair<int32_t, int32_t> cartesianCoords(
        Units::PhaseTurnsUQ16_16 angle_q16,
        Units::FractQ0_16 radius
    ) {
        // FastLED trig expects a 16-bit angle sample where 65536 == one turn. Phase must have been promoted
        // (AngleTurns16 << 16); otherwise the angle collapses to zero and artifacts appear.
        Units::AngleTurns16 angle_turns = Units::phaseQ16_16ToAngleTurns(angle_q16);
        Units::TrigQ1_15 cos_val = cos16(angle_turns);
        Units::TrigQ1_15 sin_val = sin16(angle_turns);
        int32_t x = scale_i32_by_f16(cos_val, radius);
        int32_t y = scale_i32_by_f16(sin_val, radius);
        return {x, y};
    }

    fl::pair<Units::PhaseTurnsUQ16_16, Units::FractQ0_16> polarCoords(fl::i32 x, fl::i32 y) {
        int32_t clampedX = constrain(x, (int32_t)INT16_MIN, (int32_t)INT16_MAX);
        int32_t clampedY = constrain(y, (int32_t)INT16_MIN, (int32_t)INT16_MAX);
        int16_t x16 = static_cast<int16_t>(clampedX);
        int16_t y16 = static_cast<int16_t>(clampedY);

        Units::AngleTurns16 angle_turns = atan2_turns_approx(y16, x16);
        // Promote the 16-bit sample to phase (AngleTurns16 in high 16 bits).
        Units::PhaseTurnsUQ16_16 angle_q16 = Units::angleTurnsToPhaseQ16_16(angle_turns);

        int32_t dx = x16;
        int32_t dy = y16;
        uint32_t radius_squared = static_cast<uint32_t>(dx * dx) + static_cast<uint32_t>(dy * dy);
        uint16_t magnitude = sqrt_u32(radius_squared);
        uint32_t radius_q16 = (static_cast<uint32_t>(magnitude) << 16) / static_cast<uint32_t>(Units::TRIG_Q1_15_MAX);
        if (radius_q16 > UINT16_MAX) radius_q16 = UINT16_MAX;
        Units::FractQ0_16 radius = static_cast<Units::FractQ0_16>(radius_q16);
        return {angle_q16, radius};
    }
}
