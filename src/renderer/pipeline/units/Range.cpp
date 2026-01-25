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

#include "renderer/pipeline/units/Range.h"
#include "renderer/pipeline/maths/AngleMaths.h"

namespace PolarShader {
    Range Range::polarRange(FracQ0_16 min, FracQ0_16 max) {
        return Range(min, max);
    }

    Range Range::scalarRange(int32_t min, int32_t max) {
        return Range(min, max);
    }

    Range Range::cartesianRange(int32_t radius) {
        return Range(radius);
    }

    Range::Range(Kind kind) : kind(kind) {
    }

    Range::Range(FracQ0_16 min, FracQ0_16 max) : Range(Kind::Polar) {
        min_frac = min;
        max_frac = max;
    }

    Range::Range(int32_t min, int32_t max) : Range(Kind::Scalar) {
        min_scalar = min;
        max_scalar = max;
    }

    Range::Range(int32_t radius) : Range(Kind::Cartesian) {
        this->radius = radius;
    }

    Range Range::timeRange(TimeMillis durationMs) {
        Range range(Kind::Time);
        range.duration_ms = durationMs;
        return range;
    }

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

    FracQ0_16 Range::map(SFracQ0_16 t) const {
        if (kind != Kind::Polar) return FracQ0_16(0);
        uint16_t min_raw = raw(min_frac);
        uint16_t max_raw = raw(max_frac);
        if (min_raw == max_raw) return min_frac;
        uint32_t full_turn = static_cast<uint32_t>(FRACT_Q0_16_MAX) + 1u;
        uint32_t span = 0;
        if (max_raw > min_raw) {
            span = static_cast<uint32_t>(max_raw - min_raw);
        } else {
            span = (full_turn - static_cast<uint32_t>(min_raw)) + static_cast<uint32_t>(max_raw);
        }
        uint32_t t_raw = wrap_frac_raw(raw(t));
        uint32_t scaled = (span * t_raw) >> 16;
        uint32_t result = static_cast<uint32_t>(min_raw) + scaled;
        if (result >= full_turn) result -= full_turn;
        return FracQ0_16(static_cast<uint16_t>(result));
    }

    int32_t Range::mapScalar(SFracQ0_16 t) const {
        if (kind != Kind::Scalar) return 0;
        int64_t span = static_cast<int64_t>(max_scalar) - static_cast<int64_t>(min_scalar);
        if (span <= 0) return min_scalar;
        uint32_t t_raw = clamp_frac_raw(raw(t));
        int64_t scaled = (span * static_cast<int64_t>(t_raw)) >> 16;
        return static_cast<int32_t>(static_cast<int64_t>(min_scalar) + scaled);
    }

    SPoint32 Range::mapCartesian(SFracQ0_16 direction, SFracQ0_16 velocity) const {
        if (kind != Kind::Cartesian) return SPoint32{0, 0};
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

        return SPoint32{
            static_cast<int32_t>(dx),
            static_cast<int32_t>(dy)
        };
    }

    TimeMillis Range::mapTime(SFracQ0_16 t) const {
        if (kind != Kind::Time) return 0;
        uint64_t scaled = (static_cast<uint64_t>(duration_ms) * static_cast<uint64_t>(clamp_frac_raw(raw(t)))) >> 16;
        return static_cast<TimeMillis>(scaled);
    }
}
