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
    void SaturatingClampPolicy::apply(UnboundedScalar &position_x,
                                      UnboundedScalar &position_y,
                                      int64_t dx_raw,
                                      int64_t dy_raw) {
        position_x = scalarClampQ16_16Raw(position_x.asRaw() + dx_raw);
        position_y = scalarClampQ16_16Raw(position_y.asRaw() + dy_raw);
    }

    void WrapAddPolicy::apply(UnboundedScalar &position_x,
                              UnboundedScalar &position_y,
                              int64_t dx_raw,
                              int64_t dy_raw) {
        RawQ16_16 new_x_raw = scalarAddWrapQ16_16(RawQ16_16(position_x.asRaw()),
                                              RawQ16_16(static_cast<int32_t>(dx_raw)));
        RawQ16_16 new_y_raw = scalarAddWrapQ16_16(RawQ16_16(position_y.asRaw()),
                                              RawQ16_16(static_cast<int32_t>(dy_raw)));
        position_x = UnboundedScalar::fromRaw(raw(new_x_raw));
        position_y = UnboundedScalar::fromRaw(raw(new_y_raw));
    }

    RadialClampPolicy::RadialClampPolicy(UnboundedScalar maxRadius)
        : max_radius(maxRadius) {
    }

    void RadialClampPolicy::apply(UnboundedScalar &position_x, UnboundedScalar &position_y) const {
        if (max_radius.asRaw() <= 0) {
            position_x = UnboundedScalar(0);
            position_y = UnboundedScalar(0);
            return;
        }

        int64_t x_raw = position_x.asRaw();
        int64_t y_raw = position_y.asRaw();

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

        // dist_sq and max_r_sq are in Q32.32 (raw Q16.16 squared); compare in the same scale.
        uint64_t dist_sq = x_abs * x_abs + y_abs * y_abs;

        uint64_t max_r = static_cast<uint64_t>(max_radius.asRaw());
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

        uint64_t factor = (max_r << 16) / dist; // Q16.16 scaling factor

        int64_t scaled_x = (position_x.asRaw() * static_cast<int64_t>(factor)) >> 16;
        int64_t scaled_y = (position_y.asRaw() * static_cast<int64_t>(factor)) >> 16;

        position_x = scalarClampQ16_16Raw(scaled_x);
        position_y = scalarClampQ16_16Raw(scaled_y);
    }

    ClampPolicy::ClampPolicy() : min_val(INT32_MIN), max_val(INT32_MAX) {
    }

    ClampPolicy::ClampPolicy(int32_t min, int32_t max) : min_val(min), max_val(max) {
    }

    ClampPolicy::ClampPolicy(UnboundedScalar min, UnboundedScalar max) : min_val(min), max_val(max) {
    }

    void ClampPolicy::apply(UnboundedScalar &position, UnboundedScalar &velocity) const {
        if (position < min_val) {
            position = min_val;
            velocity = UnboundedScalar(0);
        } else if (position > max_val) {
            position = max_val;
            velocity = UnboundedScalar(0);
        }
    }

    WrapPolicy::WrapPolicy(int32_t wrap) : wrap_units(wrap) {
    }

    void WrapPolicy::apply(UnboundedScalar &position, UnboundedScalar &) const {
        if (wrap_units <= 0) return;

        const int64_t wrap_q16_16 = static_cast<int64_t>(wrap_units) << 16;
        int64_t pos64 = position.asRaw();

        if (pos64 >= wrap_q16_16 || pos64 < 0) {
            pos64 %= wrap_q16_16;
            if (pos64 < 0) {
                pos64 += wrap_q16_16;
            }
        }
        position = UnboundedScalar::fromRaw(static_cast<int32_t>(pos64));
    }

    void AngleWrapPolicy::apply(UnboundedAngle &phase, RawQ16_16 phase_advance) {
        phase = phaseWrapAddSigned(phase, raw(phase_advance));
    }
}
