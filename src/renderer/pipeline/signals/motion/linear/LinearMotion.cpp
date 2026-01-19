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

#include "LinearMotion.h"
#include "renderer/pipeline/utils/MathUtils.h"
#include "renderer/pipeline/utils/TimeUtils.h"

namespace PolarShader {
    LinearMotion::LinearMotion(
        FracQ16_16 initialX,
        FracQ16_16 initialY,
        ScalarModulator speed,
        AngleModulator direction,
        bool clampEnabled,
        FracQ16_16 maxRadius
    ) : positionX(initialX),
        positionY(initialY),
        speed(std::move(speed)),
        direction(std::move(direction)),
        clampEnabled(clampEnabled),
        maxRadius(maxRadius) {
    }

    void LinearMotion::advanceFrame(TimeMillis timeInMillis) {
        if (!hasLastTime) {
            lastTime = timeInMillis;
            hasLastTime = true;
            return;
        }
        TimeMillis deltaTime = timeInMillis - lastTime;
        lastTime = timeInMillis;
        if (deltaTime == 0) return;

        deltaTime = clampDeltaTime(deltaTime);
        if (deltaTime == 0) return;

        FracQ16_16 dt_q16 = millisToQ16_16(deltaTime);
        FracQ16_16 speed_now = speed(timeInMillis);
        if (speed_now.asRaw() < 0) {
            speed_now = FracQ16_16(0);
        }

        FracQ16_16 distance = mul_q16_16_sat(speed_now, dt_q16);
        AngleTurnsUQ16_16 phase = direction(timeInMillis);
        TrigQ1_15 cos_val = cosQ1_15(phase);
        TrigQ1_15 sin_val = sinQ1_15(phase);
        int64_t dx_raw = scale_q16_16_by_trig(RawQ16_16(distance.asRaw()), cos_val);
        int64_t dy_raw = scale_q16_16_by_trig(RawQ16_16(distance.asRaw()), sin_val);

        if (clampEnabled) {
            FracQ16_16 new_x = clamp_q16_16_raw(positionX.asRaw() + dx_raw);
            FracQ16_16 new_y = clamp_q16_16_raw(positionY.asRaw() + dy_raw);
            positionX = new_x;
            positionY = new_y;
            applyRadialClamp();
        } else {
            RawQ16_16 new_x_raw = add_wrap_q16_16(RawQ16_16(positionX.asRaw()),
                                                  RawQ16_16(static_cast<int32_t>(dx_raw)));
            RawQ16_16 new_y_raw = add_wrap_q16_16(RawQ16_16(positionY.asRaw()),
                                                  RawQ16_16(static_cast<int32_t>(dy_raw)));
            positionX = FracQ16_16::fromRaw(raw(new_x_raw));
            positionY = FracQ16_16::fromRaw(raw(new_y_raw));
        }
    }

    void LinearMotion::applyRadialClamp() {
        if (!clampEnabled) return;

        if (maxRadius.asRaw() <= 0) {
            positionX = FracQ16_16(0);
            positionY = FracQ16_16(0);
            return;
        }

        int64_t x_raw = positionX.asRaw();
        int64_t y_raw = positionY.asRaw();

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

        uint64_t max_r = static_cast<uint64_t>(maxRadius.asRaw());
        uint64_t max_r_sq = max_r * max_r;

        if (dist_sq <= max_r_sq) {
            return;
        }

        uint64_t dist = sqrt_u64(dist_sq);
        if (dist == 0) {
            positionX = FracQ16_16(0);
            positionY = FracQ16_16(0);
            return;
        }

        uint64_t factor = (max_r << 16) / dist; // Q16.16 scaling factor

        int64_t scaled_x = (positionX.asRaw() * static_cast<int64_t>(factor)) >> 16;
        int64_t scaled_y = (positionY.asRaw() * static_cast<int64_t>(factor)) >> 16;

        positionX = clamp_q16_16_raw(scaled_x);
        positionY = clamp_q16_16_raw(scaled_y);
    }
}
