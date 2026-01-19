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
#pragma once

#ifndef POLAR_SHADER_PIPELINE_UTILS_FIXMATHUTILS_H
#define POLAR_SHADER_PIPELINE_UTILS_FIXMATHUTILS_H

#include "Units.h"

namespace PolarShader {
    // Constant conversion factor: milliseconds per second.
    inline constexpr uint16_t MILLIS_PER_SECOND = 1000;

    /**
     * @brief Converts milliseconds to a Q16.16 fixed-point representation of seconds.
     *
     * Use when:
     * - You need dt in seconds for Q16.16 arithmetic (e.g., speed * dt).
     *
     * Avoid when:
     * - You need a raw millisecond integer; keep TimeMillis instead.
     *
     * Notes:
     * - Uses 64-bit intermediates and rounds to the nearest Q16.16 value.
     */
    inline FracQ16_16 millisToQ16_16(TimeMillis millis) {
        // (millis / 1000) * 2^16
        // To maintain precision, this is calculated as:
        // (millis * 2^16) / 1000
        // with rounding.
        int64_t dt_raw = (static_cast<int64_t>(millis) << 16) + (MILLIS_PER_SECOND / 2);
        dt_raw /= MILLIS_PER_SECOND;
        return FracQ16_16::fromRaw(static_cast<int32_t>(dt_raw));
    }

    /**
     * @brief Create a Q16.16 value from a rational fraction without floating point.
     *
     * Use when:
     * - You need a compile-time or constexpr Q16.16 from num/den.
     *
     * Avoid when:
     * - den can be zero without a safe fallback; this returns 0 on den==0.
     */
    constexpr FracQ16_16 q16_frac(int32_t num, uint32_t den) {
        if (den == 0) return FracQ16_16(0);
        return FracQ16_16::fromRaw(static_cast<int32_t>((static_cast<int64_t>(num) << 16) / static_cast<int64_t>(den)));
    }

    /**
     * @brief Create a Q16.16 value from integer and fractional parts without floating point.
     *
     * Parameters:
     * - integer: signed integer part.
     * - fractional: Q0.16 fractional part (0..0xFFFF).
     *
     * Behavior:
     * - If integer is negative, fractional is subtracted (e.g., -2 + 0.25 -> -2.25).
     *
     * Use when:
     * - You need explicit control over the fractional bits.
     */
    constexpr FracQ16_16 q16_parts(int32_t integer, uint16_t fractional) {
        int32_t raw = static_cast<int32_t>(integer << 16);
        raw = (integer < 0) ? static_cast<int32_t>(raw - fractional)
                            : static_cast<int32_t>(raw + fractional);
        return FracQ16_16::fromRaw(raw);
    }

    /**
     * @brief Create an AngleTurnsUQ16_16 value from a rational fraction without floating point.
     *
     * Use when:
     * - You need a phase in turns from num/den (e.g., 1/4 turn).
     *
     * Avoid when:
     * - den can be zero without a safe fallback; this returns 0 on den==0.
     */
    constexpr AngleTurnsUQ16_16 angleTurns_frac(uint32_t num, uint32_t den) {
        if (den == 0) return AngleTurnsUQ16_16(0);
        return AngleTurnsUQ16_16(static_cast<uint32_t>((static_cast<uint64_t>(num) << 16) / den));
    }

    /**
     * @brief Create an AngleTurnsUQ16_16 value from integer and fractional parts without floating point.
     *
     * Parameters:
     * - integer: unsigned integer turns.
     * - fractional: Q0.16 fractional turns (0..0xFFFF).
     *
     * Use when:
     * - You need explicit control over turn fractions without float math.
     */
    constexpr AngleTurnsUQ16_16 angleTurns_parts(uint32_t integer, uint16_t fractional) {
        return AngleTurnsUQ16_16(static_cast<uint32_t>((integer << 16) | fractional));
    }
}

#endif //POLAR_SHADER_PIPELINE_UTILS_FIXMATHUTILS_H
