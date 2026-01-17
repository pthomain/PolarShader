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

#include "LinearVector.h"
#include "polar/pipeline/utils/MathUtils.h"

namespace LEDSegments {

    LinearVector::LinearVector()
        : x(FracQ16_16(0)),
          y(FracQ16_16(0)),
          speed(FracQ16_16(0)),
          direction(AngleUnitsQ0_16(0)) {
    }

    LinearVector::LinearVector(FracQ16_16 x,
                               FracQ16_16 y,
                               FracQ16_16 speed,
                               AngleUnitsQ0_16 direction)
        : x(x),
          y(y),
          speed(speed),
          direction(direction) {
    }

    LinearVector LinearVector::fromVelocity(FracQ16_16 speed,
                                            AngleUnitsQ0_16 direction) {
        TrigQ1_15 cos_val = cosQ1_15(direction);
        TrigQ1_15 sin_val = sinQ1_15(direction);
        RawQ16_16 speed_raw = RawQ16_16(speed.asRaw());
        RawQ16_16 x_raw = RawQ16_16(static_cast<int32_t>(
            scale_q16_16_by_trig(speed_raw, cos_val)));
        RawQ16_16 y_raw = RawQ16_16(static_cast<int32_t>(
            scale_q16_16_by_trig(speed_raw, sin_val)));
        return LinearVector(FracQ16_16::fromRaw(raw(x_raw)),
                            FracQ16_16::fromRaw(raw(y_raw)),
                            speed,
                            direction);
    }

    LinearVector LinearVector::fromXY(FracQ16_16 x, FracQ16_16 y) {
        RawQ16_16 x_raw = RawQ16_16(x.asRaw());
        RawQ16_16 y_raw = RawQ16_16(y.asRaw());

        uint64_t x_abs = static_cast<uint64_t>(
            raw(x_raw) == INT32_MIN ? INT32_MAX : (raw(x_raw) < 0 ? -raw(x_raw) : raw(x_raw)));
        uint64_t y_abs = static_cast<uint64_t>(
            raw(y_raw) == INT32_MIN ? INT32_MAX : (raw(y_raw) < 0 ? -raw(y_raw) : raw(y_raw)));
        uint64_t sum = x_abs * x_abs + y_abs * y_abs;
        uint64_t speed_raw_u64 = sqrt_u64(sum);
        RawQ16_16 speed_raw = (speed_raw_u64 > static_cast<uint64_t>(INT32_MAX))
                                      ? RawQ16_16(INT32_MAX)
                                      : RawQ16_16(static_cast<int32_t>(speed_raw_u64));
        FracQ16_16 speed = FracQ16_16::fromRaw(raw(speed_raw));
        AngleUnitsQ0_16 direction = atan2_turns_approx(clamp_raw_to_i16(y_raw),
                                                       clamp_raw_to_i16(x_raw));

        return LinearVector(x, y, speed, direction);
    }
}
