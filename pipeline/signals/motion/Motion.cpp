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

#include "Motion.h"

namespace LEDSegments {
    LinearMotion::LinearMotion(
        FracQ16_16 initialX,
        FracQ16_16 initialY,
        Modulation<LinearVector> velocity,
        bool clampEnabled,
        FracQ16_16 maxRadius
    ) : positionX(initialX),
        positionY(initialY),
        velocity(std::move(velocity)),
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
        LinearVector velocity_now = velocity(timeInMillis);
        RawQ16_16 dx_raw = RawQ16_16(mul_q16_16_sat(velocity_now.getX(), dt_q16).asRaw());
        RawQ16_16 dy_raw = RawQ16_16(mul_q16_16_sat(velocity_now.getY(), dt_q16).asRaw());

        if (clampEnabled) {
            int64_t new_x_raw = clamp_raw(static_cast<int64_t>(positionX.asRaw()) + raw(dx_raw));
            int64_t new_y_raw = clamp_raw(static_cast<int64_t>(positionY.asRaw()) + raw(dy_raw));
            positionX = FracQ16_16::fromRaw(static_cast<int32_t>(new_x_raw));
            positionY = FracQ16_16::fromRaw(static_cast<int32_t>(new_y_raw));
            applyRadialClamp();
        } else {
            RawQ16_16 new_x_raw = add_wrap_q16_16(RawQ16_16(positionX.asRaw()), dx_raw);
            RawQ16_16 new_y_raw = add_wrap_q16_16(RawQ16_16(positionY.asRaw()), dy_raw);
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
            x_raw == INT32_MIN ? INT32_MAX : (x_raw < 0 ? -x_raw : x_raw));
        uint64_t y_abs = static_cast<uint64_t>(
            y_raw == INT32_MIN ? INT32_MAX : (y_raw < 0 ? -y_raw : y_raw));
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

        positionX = FracQ16_16::fromRaw(static_cast<int32_t>(clamp_raw(scaled_x)));
        positionY = FracQ16_16::fromRaw(static_cast<int32_t>(clamp_raw(scaled_y)));
    }

    AngularMotion::AngularMotion(AngleUnitsQ0_16 initial,
                                 Modulation<FracQ16_16> speed)
        : phase(angleUnitsToAngleTurns(initial)),
          speed(std::move(speed)) {
    }

    void AngularMotion::advanceFrame(TimeMillis timeInMillis) {
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
        RawQ16_16 phase_advance = RawQ16_16(mul_q16_16_wrap(speed(timeInMillis), dt_q16).asRaw());
        phase = wrapAddSigned(phase, raw(phase_advance));
    }

    ScalarMotion::ScalarMotion(Modulation<FracQ16_16> delta)
        : delta(std::move(delta)) {
    }

    void ScalarMotion::advanceFrame(TimeMillis timeInMillis) {
        value = delta(timeInMillis);
    }
}
