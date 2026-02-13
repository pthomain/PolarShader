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
#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif
#include "renderer/pipeline/maths/AngleMaths.h"
#include "renderer/pipeline/maths/ScalarMaths.h"

namespace PolarShader {

    namespace PolarMaths {
        f16 shortest_angle_dist(f16 a, f16 b) {
            uint16_t dist = raw(a) > raw(b) ? raw(a) - raw(b) : raw(b) - raw(a);
            if (dist > U16_HALF) {
                uint32_t wrap = ANGLE_FULL_TURN_U32 - dist;
                dist = static_cast<uint16_t>(wrap);
            }
            return f16(dist);
        }
    }

    namespace {
        constexpr f16 angleAtan2TurnsApprox(int16_t y, int16_t x) {
            if (x == 0 && y == 0) return f16(0);

            uint16_t abs_x = (x < 0) ? static_cast<uint16_t>(-x) : static_cast<uint16_t>(x);
            uint16_t abs_y = (y < 0) ? static_cast<uint16_t>(-y) : static_cast<uint16_t>(y);

            uint16_t max_val = (abs_x > abs_y) ? abs_x : abs_y;
            uint16_t min_val = (abs_x > abs_y) ? abs_y : abs_x;

            uint32_t z = (static_cast<uint32_t>(min_val) << 16) / max_val; // f16/sf16
            uint32_t one_minus_z = ANGLE_FULL_TURN_U32 - z;

            constexpr uint32_t A_Q16 = ANGLE_FULL_TURN_U32 / 8; // 0.125 turns in f16/sf16
            constexpr uint32_t B_Q16 = 2847u; // 0.04345 turns in f16/sf16

            uint32_t inner = A_Q16 + ((B_Q16 * one_minus_z) >> 16);
            uint32_t base = (z * inner) >> 16; // 0..0.125 turns

            uint32_t angle = (abs_x >= abs_y) ? base : (QUARTER_TURN_U16 - base);

            if (x < 0) angle = HALF_TURN_U16 - angle;
            if (y < 0) angle = ANGLE_FULL_TURN_U32 - angle;

            return f16(static_cast<uint16_t>(angle & ANGLE_U16_MAX));
        }
    }

    UV polarToCartesianUV(UV polar_uv) {
        f16 angle_turns = f16(static_cast<uint16_t>(raw(polar_uv.u)));
        f16 radius = f16(static_cast<uint16_t>(raw(polar_uv.v)));

        sf16 cos_val = angleCosF16(angle_turns);
        sf16 sin_val = angleSinF16(angle_turns);

        // x = cos(angle) * radius, y = sin(angle) * radius
        // Result is in range [-1, 1] relative to center
        int32_t x_raw = mulI32F16Sat(raw(cos_val), radius);
        int32_t y_raw = mulI32F16Sat(raw(sin_val), radius);

        // Map from [-1, 1] to [0, 1]
        // x_normalized = (x + 1) / 2
        // (x_raw + 65536) / 2
        int32_t x_norm = (x_raw + 0x00010000) >> 1;
        int32_t y_norm = (y_raw + 0x00010000) >> 1;

        return UV(sr16(x_norm), sr16(y_norm));
    }

    UV cartesianToPolarUV(UV cart_uv) {
        // Map from [0, 1] to [-1, 1]
        // x = cart_u * 2 - 1
        int32_t x = (raw(cart_uv.u) << 1) - 0x00010000;
        int32_t y = (raw(cart_uv.v) << 1) - 0x00010000;

        int32_t clamped_x = constrain(x, SF16_MIN, SF16_MAX);
        int32_t clamped_y = constrain(y, SF16_MIN, SF16_MAX);
        int16_t x16 = static_cast<int16_t>(clamped_x >> 1);
        int16_t y16 = static_cast<int16_t>(clamped_y >> 1);

        f16 angle_units = angleAtan2TurnsApprox(y16, x16);
        int64_t dx = clamped_x;
        int64_t dy = clamped_y;

        uint64_t radius_squared = static_cast<uint64_t>(dx * dx) + static_cast<uint64_t>(dy * dy);
        uint64_t magnitude_raw = sqrtU64Raw(radius_squared);

        if (magnitude_raw > static_cast<uint64_t>(SF16_MAX)) {
            magnitude_raw = SF16_MAX;
        }

        return UV(sr16(raw(angle_units)), sr16(static_cast<int32_t>(magnitude_raw)));
    }
}
