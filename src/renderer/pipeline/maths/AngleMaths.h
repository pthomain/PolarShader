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

#ifndef POLAR_SHADER_PIPELINE_MATHS_ANGLEMATHS_H
#define POLAR_SHADER_PIPELINE_MATHS_ANGLEMATHS_H

#if defined(ARDUINO) || defined(__EMSCRIPTEN__)
#include "FastLED.h"
#else
#include "native/FastLED.h"
#endif
#include "renderer/pipeline/maths/units/Units.h"

namespace PolarShader {
    constexpr u0x16 angleFrac(uint32_t denominator) {
        if (denominator == 0) return u0x16(0);
        return u0x16(static_cast<uint32_t>((static_cast<uint64_t>(ANGLE_FULL_TURN_U32)) / denominator));
    }

    constexpr u0x16 angleFrac360(uint16_t angleDegrees) {
        if (angleDegrees == 0) return u0x16(0);
        return u0x16(static_cast<uint32_t>((static_cast<uint64_t>(U0X16_MAX) * angleDegrees) / 360));
    }

    // FastLED trig sampling helpers (explicit angle/phase conversions).
    constexpr uint16_t angleToFastLedPhase(u0x16 a) { return raw(a); }

    // --- Angle wrap arithmetic (mod 2^16) ---
    constexpr u0x16 angleWrapAdd(u0x16 a, uint16_t delta) {
        return u0x16(static_cast<uint16_t>(raw(a) + delta));
    }

    // Wrap-add using signed raw delta (u0x16/s0x16), interpreted via two's-complement.
    constexpr u0x16 angleWrapAddSigned(u0x16 a, int32_t delta_raw_q0_16) {
        uint32_t sum = static_cast<uint32_t>(raw(a)) + static_cast<uint32_t>(delta_raw_q0_16);
        return u0x16(static_cast<uint16_t>(sum));
    }

    inline s0x16 angleSinU0x16(u0x16 a) {
        int16_t raw_q1_15 = sin16(angleToFastLedPhase(a));
        return s0x16(static_cast<int32_t>(raw_q1_15) << 1);
    }

    inline s0x16 angleCosU0x16(u0x16 a) {
        int16_t raw_q1_15 = cos16(angleToFastLedPhase(a));
        return s0x16(static_cast<int32_t>(raw_q1_15) << 1);
    }

    // Approximate atan2 in u0x16 turns. Uses a Chebyshev-coefficient polynomial
    // (B_Q16 = 2847, ~0.043 rad max error) over the [0, pi/4] octant, then
    // mirrors/quadrant-corrects. inputs are int16 components; signs select
    // the quadrant.
    constexpr u0x16 angleAtan2TurnsApprox(int16_t y, int16_t x) {
        if (x == 0 && y == 0) return u0x16(0);

        uint16_t abs_x = (x < 0) ? static_cast<uint16_t>(-x) : static_cast<uint16_t>(x);
        uint16_t abs_y = (y < 0) ? static_cast<uint16_t>(-y) : static_cast<uint16_t>(y);

        uint16_t max_val = (abs_x > abs_y) ? abs_x : abs_y;
        uint16_t min_val = (abs_x > abs_y) ? abs_y : abs_x;

        uint32_t z = (static_cast<uint32_t>(min_val) << 16) / max_val; // u0x16/s0x16
        uint32_t one_minus_z = ANGLE_FULL_TURN_U32 - z;

        constexpr uint32_t A_Q16 = ANGLE_FULL_TURN_U32 / 8; // 0.125 turns in u0x16/s0x16
        constexpr uint32_t B_Q16 = 2847u; // 0.04345 turns in u0x16/s0x16

        uint32_t inner = A_Q16 + ((B_Q16 * one_minus_z) >> 16);
        uint32_t base = (z * inner) >> 16; // 0..0.125 turns

        uint32_t angle = (abs_x >= abs_y) ? base : (QUARTER_TURN_U16 - base);

        if (x < 0) angle = HALF_TURN_U16 - angle;
        if (y < 0) angle = ANGLE_FULL_TURN_U32 - angle;

        return u0x16(static_cast<uint16_t>(angle & ANGLE_U16_MAX));
    }
}

#endif // POLAR_SHADER_PIPELINE_MATHS_ANGLEMATHS_H
