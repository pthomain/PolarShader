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

#include "BoundPolicies.h"

#include "FastLED.h"
#include "renderer/pipeline/maths/Maths.h"
#include "renderer/pipeline/units/Units.h"

namespace PolarShader {
    void SaturatingClampPolicy::apply(
        UnboundedScalar &position_x,
        UnboundedScalar &position_y,
        int32_t dx_raw,
        int32_t dy_raw
    ) {
        position_x = scalarClampQ0_16Raw(static_cast<int64_t>(raw(position_x)) + dx_raw);
        position_y = scalarClampQ0_16Raw(static_cast<int64_t>(raw(position_y)) + dy_raw);
    }

    void WrapAddPolicy::apply(
        UnboundedScalar &position_x,
        UnboundedScalar &position_y,
        int32_t dx_raw,
        int32_t dy_raw
    ) {
        uint32_t sum_x = static_cast<uint32_t>(raw(position_x)) + static_cast<uint32_t>(dx_raw);
        uint32_t sum_y = static_cast<uint32_t>(raw(position_y)) + static_cast<uint32_t>(dy_raw);
        position_x = UnboundedScalar(static_cast<int32_t>(sum_x));
        position_y = UnboundedScalar(static_cast<int32_t>(sum_y));
    }

    RadialClampPolicy::RadialClampPolicy(UnboundedScalar maxRadius)
        : max_radius(maxRadius) {
    }

    void RadialClampPolicy::apply(
        UnboundedScalar &position_x,
        UnboundedScalar &position_y
    ) const {
        if (raw(max_radius) <= 0) {
            position_x = UnboundedScalar(0);
            position_y = UnboundedScalar(0);
            return;
        }

        int64_t x_raw = raw(position_x);
        int64_t y_raw = raw(position_y);

        uint64_t x_abs = static_cast<uint64_t>(
            x_raw == INT32_MIN
                ? INT32_MAX
                : (x_raw < 0 ? -x_raw : x_raw)
        );

        uint64_t y_abs = static_cast<uint64_t>(
            y_raw == INT32_MIN
                ? INT32_MAX
                : (y_raw < 0 ? -y_raw : y_raw)
        );

        // dist_sq and max_r_sq are in raw Q0.16 squared; compare in the same scale.
        uint64_t dist_sq = x_abs * x_abs + y_abs * y_abs;

        uint64_t max_r = static_cast<uint64_t>(raw(max_radius));
        uint64_t max_r_sq = max_r * max_r;

        if (dist_sq <= max_r_sq) {
            return;
        }

        uint64_t dist = scalarSqrtU64(dist_sq);
        if (dist == 0) {
            position_x = UnboundedScalar(0);
            position_y = UnboundedScalar(0);
            return;
        }

        uint64_t factor = (max_r << 16) / dist; // Q0.16 scaling factor

        int64_t scaled_x = (static_cast<int64_t>(raw(position_x)) * static_cast<int64_t>(factor)) >> 16;
        int64_t scaled_y = (static_cast<int64_t>(raw(position_y)) * static_cast<int64_t>(factor)) >> 16;

        position_x = scalarClampQ0_16Raw(scaled_x);
        position_y = scalarClampQ0_16Raw(scaled_y);
    }

    ClampPolicy::ClampPolicy() : min_val(Q0_16_MIN), max_val(Q0_16_MAX) {
    }

    ClampPolicy::ClampPolicy(int32_t min, int32_t max) : min_val(min), max_val(max) {
    }

    ClampPolicy::ClampPolicy(UnboundedScalar min, UnboundedScalar max) : min_val(min), max_val(max) {
    }

    void ClampPolicy::apply(UnboundedScalar &position, UnboundedScalar &velocity) const {
        if (raw(position) < raw(min_val)) {
            position = min_val;
            velocity = UnboundedScalar(0);
        } else if (raw(position) > raw(max_val)) {
            position = max_val;
            velocity = UnboundedScalar(0);
        }
    }

    WrapPolicy::WrapPolicy(int32_t wrap) : wrap_units(wrap) {
    }

    void WrapPolicy::apply(UnboundedScalar &position, UnboundedScalar &) const {
        if (wrap_units <= 0) return;

        const int64_t wrap_raw = wrap_units;
        int64_t pos64 = raw(position);

        if (pos64 >= wrap_raw || pos64 < 0) {
            pos64 %= wrap_raw;
            if (pos64 < 0) {
                pos64 += wrap_raw;
            }
        }
        position = UnboundedScalar(static_cast<int32_t>(pos64));
    }

    void AngleWrapPolicy::apply(AngleQ0_16 &phase, int32_t phase_advance_raw) {
        phase = angleWrapAddSigned(phase, phase_advance_raw);
    }
}
