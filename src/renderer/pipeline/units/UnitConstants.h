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

#ifndef POLAR_SHADER_PIPELINE_UNITS_UNITCONSTANTS_H
#define POLAR_SHADER_PIPELINE_UNITS_UNITCONSTANTS_H

#include <cstdint>

namespace PolarShader {
    // Represents the midpoint of a 16-bit unsigned integer range, often used for remapping.
    inline constexpr uint16_t U16_HALF = 0x8000;

    // The maximum positive value for a Q1.15 signed integer, used in FastLED's trig functions.
    inline constexpr int16_t TRIG_Q1_15_MAX = 32767;

    // The maximum value for a Q0.16 unsigned fraction, representing the value closest to 1.0.
    inline constexpr uint16_t FRACT_Q0_16_MAX = 0xFFFF;

    // Scalar 1.0 in Q16.16 fixed-point format.
    inline constexpr uint32_t Q16_16_ONE = 1u << 16;

    // A full revolution in the phase accumulator: angle units stored in the high 16 bits, wraps at 2^32.
    inline constexpr uint64_t PHASE_FULL_TURN = 1ULL << 32;

    // --- Angle domain constants (uint16_t) ---
    // The domain for 16-bit angle samples is [0..65535], where 65536 represents a full circle (the modulus).
    inline constexpr uint16_t QUARTER_TURN_U16 = 16384u;
    inline constexpr uint16_t HALF_TURN_U16 = 32768u;
    inline constexpr uint16_t ANGLE_U16_MAX = 65535u; // The maximum representable value for a uint16_t angle.
}

#endif // POLAR_SHADER_PIPELINE_UNITS_UNITCONSTANTS_H
