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
#if defined(ARDUINO) || defined(__EMSCRIPTEN__)
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif
#include "renderer/pipeline/maths/AngleMaths.h"
#include "renderer/pipeline/maths/ScalarMaths.h"

namespace PolarShader {

    namespace PolarMaths {
        u0x16 shortest_angle_dist(u0x16 a, u0x16 b) {
            uint16_t dist = raw(a) > raw(b) ? raw(a) - raw(b) : raw(b) - raw(a);
            if (dist > U16_HALF) {
                uint32_t wrap = ANGLE_FULL_TURN_U32 - dist;
                dist = static_cast<uint16_t>(wrap);
            }
            return u0x16(dist);
        }
    }

    UV polarToCartesianUV(UV polar_uv) {
        u0x16 angle_turns = u0x16(static_cast<uint16_t>(polar_uv.u.raw()));
        u0x16 radius = u0x16(static_cast<uint16_t>(polar_uv.v.raw()));

        s0x16 cos_val = angleCosU0x16(angle_turns);
        s0x16 sin_val = angleSinU0x16(angle_turns);

        // x = cos(angle) * radius, y = sin(angle) * radius
        // Result is in range [-1, 1] relative to center
        int32_t x_raw = mulI32U0x16Sat(raw(cos_val), radius);
        int32_t y_raw = mulI32U0x16Sat(raw(sin_val), radius);

        // Map from [-1, 1] to [0, 1]
        // x_normalized = (x + 1) / 2
        // (x_raw + 65536) / 2
        int32_t x_norm = (x_raw + 0x00010000) >> 1;
        int32_t y_norm = (y_raw + 0x00010000) >> 1;

        return UV(fl::s16x16::from_raw(x_norm), fl::s16x16::from_raw(y_norm));
    }

    UV cartesianToPolarUV(UV cart_uv) {
        // Map from [0, 1] to [-1, 1]
        // x = cart_u * 2 - 1
        int32_t x = (cart_uv.u.raw() << 1) - 0x00010000;
        int32_t y = (cart_uv.v.raw() << 1) - 0x00010000;

        int32_t clamped_x = constrain(x, S0X16_MIN, S0X16_MAX);
        int32_t clamped_y = constrain(y, S0X16_MIN, S0X16_MAX);
        int16_t x16 = static_cast<int16_t>(clamped_x >> 1);
        int16_t y16 = static_cast<int16_t>(clamped_y >> 1);

        u0x16 angle_units = angleAtan2TurnsApprox(y16, x16);
        int64_t dx = clamped_x;
        int64_t dy = clamped_y;

        uint64_t radius_squared = static_cast<uint64_t>(dx * dx) + static_cast<uint64_t>(dy * dy);
        uint64_t magnitude_raw = sqrtU64Raw(radius_squared);

        if (magnitude_raw > static_cast<uint64_t>(S0X16_MAX)) {
            magnitude_raw = S0X16_MAX;
        }

        return UV(fl::s16x16::from_raw(raw(angle_units)), fl::s16x16::from_raw(static_cast<int32_t>(magnitude_raw)));
    }
}
