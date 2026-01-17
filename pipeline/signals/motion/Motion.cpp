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
        Units::FracQ16_16 initialX,
        Units::FracQ16_16 initialY,
        Fluctuation<LinearVector> velocity,
        bool clampEnabled,
        Units::FracQ16_16 maxRadius
    ) : positionX(initialX),
        positionY(initialY),
        velocity(std::move(velocity)),
        clampEnabled(clampEnabled),
        maxRadius(maxRadius) {
    }

    void LinearMotion::advanceFrame(Units::TimeMillis timeInMillis) {
        if (lastTime == 0) {
            lastTime = timeInMillis;
            return;
        }
        Units::TimeMillis deltaTime = timeInMillis - lastTime;
        lastTime = timeInMillis;
        if (deltaTime == 0) return;

        deltaTime = Fluctuations::clampDeltaTime(deltaTime);
        if (deltaTime == 0) return;

        Units::FracQ16_16 dt_q16 = millisToQ16_16(deltaTime);
        LinearVector velocity_now = velocity(timeInMillis);
        Units::RawFracQ16_16 dx_raw = mul_q16_16_sat(velocity_now.getX(), dt_q16).asRaw();
        Units::RawFracQ16_16 dy_raw = mul_q16_16_sat(velocity_now.getY(), dt_q16).asRaw();

        if (clampEnabled) {
            int64_t new_x_raw = clamp_raw(static_cast<int64_t>(positionX.asRaw()) + dx_raw);
            int64_t new_y_raw = clamp_raw(static_cast<int64_t>(positionY.asRaw()) + dy_raw);
            positionX = Units::FracQ16_16::fromRaw(static_cast<int32_t>(new_x_raw));
            positionY = Units::FracQ16_16::fromRaw(static_cast<int32_t>(new_y_raw));
            applyRadialClamp();
        } else {
            Units::RawFracQ16_16 new_x_raw = add_wrap_q16_16(positionX.asRaw(), dx_raw);
            Units::RawFracQ16_16 new_y_raw = add_wrap_q16_16(positionY.asRaw(), dy_raw);
            positionX = Units::FracQ16_16::fromRaw(new_x_raw);
            positionY = Units::FracQ16_16::fromRaw(new_y_raw);
        }
    }

    void LinearMotion::applyRadialClamp() {
        if (!clampEnabled) return;

        if (maxRadius.asRaw() <= 0) {
            positionX = Units::FracQ16_16(0);
            positionY = Units::FracQ16_16(0);
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
            positionX = Units::FracQ16_16(0);
            positionY = Units::FracQ16_16(0);
            return;
        }

        uint64_t factor = (max_r << 16) / dist; // Q16.16 scaling factor

        int64_t scaled_x = (positionX.asRaw() * static_cast<int64_t>(factor)) >> 16;
        int64_t scaled_y = (positionY.asRaw() * static_cast<int64_t>(factor)) >> 16;

        positionX = Units::FracQ16_16::fromRaw(static_cast<int32_t>(clamp_raw(scaled_x)));
        positionY = Units::FracQ16_16::fromRaw(static_cast<int32_t>(clamp_raw(scaled_y)));
    }

    AngularMotion::AngularMotion(Units::AngleUnitsQ0_16 initial,
                                 Fluctuation<Units::FracQ16_16> speed)
        : phase(Units::angleUnitsToAngleTurns(initial)),
          speed(std::move(speed)) {
    }

    void AngularMotion::advanceFrame(Units::TimeMillis timeInMillis) {
        if (lastTime == 0) {
            lastTime = timeInMillis;
            return;
        }
        Units::TimeMillis deltaTime = timeInMillis - lastTime;
        lastTime = timeInMillis;
        if (deltaTime == 0) return;

        deltaTime = Fluctuations::clampDeltaTime(deltaTime);
        if (deltaTime == 0) return;

        Units::FracQ16_16 dt_q16 = millisToQ16_16(deltaTime);
        Units::RawFracQ16_16 phase_advance = mul_q16_16_wrap(speed(timeInMillis), dt_q16).asRaw();
        phase += static_cast<uint32_t>(phase_advance);
    }

    ScalarMotion::ScalarMotion(Fluctuation<Units::FracQ16_16> delta)
        : delta(std::move(delta)) {
    }

    void ScalarMotion::advanceFrame(Units::TimeMillis timeInMillis) {
        value = delta(timeInMillis);
    }
}
