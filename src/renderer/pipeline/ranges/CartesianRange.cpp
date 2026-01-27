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

#include "CartesianRange.h"
#include "renderer/pipeline/maths/AngleMaths.h"

namespace PolarShader {
    namespace {
        uint32_t clamp_frac_raw(int32_t raw_value) {
            if (raw_value <= 0) return 0u;
            if (raw_value >= static_cast<int32_t>(FRACT_Q0_16_MAX)) return FRACT_Q0_16_MAX;
            return static_cast<uint32_t>(raw_value);
        }

        uint32_t wrap_frac_raw(int32_t raw_value) {
            return static_cast<uint32_t>(raw_value) & 0xFFFFu;
        }
    }

    CartesianRange::CartesianRange(int32_t radius)
        : radius(radius) {
    }

    MappedSignal<SPoint32> CartesianRange::map(SFracQ0_16 direction, SFracQ0_16 velocity) const {
        int32_t vel_raw_signed = raw(velocity);
        bool negate = vel_raw_signed < 0;
        int32_t vel_mag_signed = negate ? -vel_raw_signed : vel_raw_signed;
        uint32_t vel_raw = clamp_frac_raw(vel_mag_signed);
        uint16_t dir_raw = static_cast<uint16_t>(wrap_frac_raw(raw(direction)));
        if (negate) {
            dir_raw = static_cast<uint16_t>(dir_raw + HALF_TURN_U16);
        }
        FracQ0_16 angle(dir_raw);
        TrigQ0_16 cos_q0_16 = angleCosQ0_16(angle);
        TrigQ0_16 sin_q0_16 = angleSinQ0_16(angle);

        int64_t scaled = static_cast<int64_t>(radius) * static_cast<int64_t>(vel_raw);
        int64_t dx = scaled * static_cast<int64_t>(raw(cos_q0_16));
        int64_t dy = scaled * static_cast<int64_t>(raw(sin_q0_16));
        dx += (dx >= 0) ? (1LL << 31) : -(1LL << 31);
        dy += (dy >= 0) ? (1LL << 31) : -(1LL << 31);
        dx >>= 32;
        dy >>= 32;

        return MappedSignal(SPoint32{
            static_cast<int32_t>(dx),
            static_cast<int32_t>(dy)
        });
    }
}
