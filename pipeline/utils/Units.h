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

#ifndef LED_SEGMENTS_PIPELINE_UTILS_UNITS_H
#define LED_SEGMENTS_PIPELINE_UTILS_UNITS_H

#include <FixMath.h>
#include "FastLED.h"

namespace LEDSegments::Units {
    // --- Constants ---

    // Represents the midpoint of a 16-bit unsigned integer range, often used for remapping.
    inline constexpr uint16_t U16_HALF = 0x8000;

    // The maximum positive value for a Q1.15 signed integer, used in FastLED's trig functions.
    inline constexpr int16_t TRIG_Q1_15_MAX = 32767;

    // The maximum value for a Q0.16 unsigned fraction, representing the value closest to 1.0.
    inline constexpr uint16_t FRACT_Q0_16_MAX = 0xFFFF;

    // Scalar 1.0 in Q16.16 fixed-point format.
    inline constexpr uint32_t Q16_16_ONE = 1u << 16;
    // A full revolution in the phase accumulator: AngleTurns16 stored in the high 16 bits, wraps at 2^32.
    inline constexpr uint64_t PHASE_FULL_TURN = 1ULL << 32;

    // --- Angle Domain Constants (uint16_t) ---
    // The domain for 16-bit angle samples is [0..65535], where 65536 represents a full circle (the modulus).
    inline constexpr uint16_t QUARTER_TURN_U16 = 16384u;
    inline constexpr uint16_t HALF_TURN_U16 = 32768u;
    inline constexpr uint16_t ANGLE_U16_MAX = 65535u; // The maximum representable value for a uint16_t angle.

    // --- Type Aliases ---
    // Angle / phase domain
    // A 16-bit value representing a fraction of a turn. The domain is [0..65535], covering the interval [0, 1).
    using AngleTurns16 = uint16_t;
    // A 32-bit accumulator where the upper 16 bits hold AngleTurns16 and the lower 16 bits are fractional turns.
    using PhaseTurnsUQ16_16 = uint32_t;

    // Trigonometry (FastLED)
    using TrigQ1_15 = int16_t; // sin16/cos16 output

    // Fractions (FastLED)
    using FractQ0_16 = fract16; // unsigned [0,1)

    // Signals (FixMath)
    using SignalQ16_16 = SFix<16, 16>; // signed Q16.16
    using RawSignalQ16_16 = int32_t; // SignalQ16_16::asRaw()
    using PhaseVelAngleUnitsQ16_16 = SignalQ16_16; // Q16.16-scaled AngleTurns16 per second (phase units/sec)

    // Coordinates
    using CartesianCoord = int32_t;

    // Noise
    using NoiseRawU16 = uint16_t; // inoise16 output
    using NoiseNormU16 = uint16_t; // normaliseNoise16 output

    // Time
    using TimeMillis = unsigned long; // Arduino millis()

    // --- Conversions ---
    inline PhaseTurnsUQ16_16 angleTurnsToPhaseQ16_16(AngleTurns16 angle) {
        return static_cast<PhaseTurnsUQ16_16>(static_cast<uint32_t>(angle) << 16);
    }

    inline AngleTurns16 phaseQ16_16ToAngleTurns(PhaseTurnsUQ16_16 phase) {
        return static_cast<AngleTurns16>(phase >> 16);
    }
}

#endif //LED_SEGMENTS_PIPELINE_UTILS_UNITS_H
