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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_MOTION_BOUNDUTILS_H
#define POLAR_SHADER_PIPELINE_SIGNALS_MOTION_BOUNDUTILS_H

#include "renderer/pipeline/utils/Units.h"

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
    inline UnboundedScalar millisToUnboundedScalar(TimeMillis millis) {
        // (millis / 1000) * 2^16
        // To maintain precision, this is calculated as:
        // (millis * 2^16) / 1000
        // with rounding.
        int64_t dt_raw = (static_cast<int64_t>(millis) << 16) + (MILLIS_PER_SECOND / 2);
        dt_raw /= MILLIS_PER_SECOND;
        return UnboundedScalar::fromRaw(static_cast<int32_t>(dt_raw));
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
    constexpr UnboundedScalar unboundedScalar(int32_t num, uint32_t den) {
        if (den == 0) return UnboundedScalar(0);
        return UnboundedScalar::fromRaw(
            static_cast<int32_t>((static_cast<int64_t>(num) << 16) / static_cast<int64_t>(den))
        );
    }

    /**
     * @brief Create a Q0.16 bounded scalar from a rational fraction without floating point.
     *
     * Use when:
     * - You need a compile-time or constexpr BoundedScalar from num/den.
     *
     * Avoid when:
     * - den can be zero without a safe fallback; this returns 0 on den==0.
     */
    constexpr BoundedScalar boundedScalar(uint32_t num, uint32_t den) {
        if (den == 0) return BoundedScalar(0);
        uint32_t raw_value = static_cast<uint32_t>((static_cast<uint64_t>(num) * FRACT_Q0_16_MAX) / den);
        return BoundedScalar(static_cast<uint16_t>(raw_value));
    }

    /**
     * @brief Create a bounded angle (AngleUnitsQ0_16) from a rational fraction without floating point.
     */
    constexpr BoundedAngle boundedAngle(uint32_t num, uint32_t den) {
        if (den == 0) return BoundedAngle(0);
        uint32_t raw_value = static_cast<uint32_t>((static_cast<uint64_t>(num) * FRACT_Q0_16_MAX) / den);
        return BoundedAngle(static_cast<uint16_t>(raw_value));
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
    constexpr UnboundedScalar unboundedScalarParts(int32_t integer, uint16_t fractional) {
        int32_t raw = integer << 16;
        raw = (integer < 0) ? raw - fractional : raw + fractional;
        return UnboundedScalar::fromRaw(raw);
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
    constexpr UnboundedAngle unboundedAngle(uint32_t num, uint32_t den) {
        if (den == 0) return UnboundedAngle(0);
        return UnboundedAngle(static_cast<uint32_t>((static_cast<uint64_t>(num) << 16) / den));
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
    constexpr UnboundedAngle unboundedAngleParts(uint32_t integer, uint16_t fractional) {
        return UnboundedAngle(integer << 16 | fractional);
    }

    UnboundedScalar unbound(BoundedScalar t, UnboundedScalar min_val, UnboundedScalar max_val);

    BoundedScalar bound(UnboundedScalar value, UnboundedScalar min_val, UnboundedScalar max_val);
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_MOTION_BOUNDUTILS_H
