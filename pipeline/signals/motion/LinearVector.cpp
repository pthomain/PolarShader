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
#include <climits>
#include "polar/pipeline/utils/MathUtils.h"

namespace LEDSegments {

    LinearVector::LinearVector(Units::FracQ16_16 x,
                               Units::FracQ16_16 y,
                               Units::FracQ16_16 speed,
                               Units::AngleUnitsQ0_16 direction)
        : x(x),
          y(y),
          speed(speed),
          direction(direction) {
    }

    LinearVector LinearVector::fromVelocity(Units::FracQ16_16 speed,
                                            Units::AngleUnitsQ0_16 direction) {
        int32_t cos_val = cos16(direction);
        int32_t sin_val = sin16(direction);
        Units::RawFracQ16_16 x_raw = static_cast<Units::RawFracQ16_16>(
            scale_q16_16_by_trig(speed.asRaw(), cos_val));
        Units::RawFracQ16_16 y_raw = static_cast<Units::RawFracQ16_16>(
            scale_q16_16_by_trig(speed.asRaw(), sin_val));
        return LinearVector(Units::FracQ16_16::fromRaw(x_raw),
                            Units::FracQ16_16::fromRaw(y_raw),
                            speed,
                            direction);
    }

    LinearVector LinearVector::fromXY(Units::FracQ16_16 x, Units::FracQ16_16 y) {
        Units::RawFracQ16_16 x_raw = x.asRaw();
        Units::RawFracQ16_16 y_raw = y.asRaw();

        uint64_t x_abs = static_cast<uint64_t>(
            x_raw == INT32_MIN ? INT32_MAX : (x_raw < 0 ? -x_raw : x_raw));
        uint64_t y_abs = static_cast<uint64_t>(
            y_raw == INT32_MIN ? INT32_MAX : (y_raw < 0 ? -y_raw : y_raw));
        uint64_t sum = x_abs * x_abs + y_abs * y_abs;
        uint64_t speed_raw_u64 = sqrt_u64(sum);
        Units::RawFracQ16_16 speed_raw = (speed_raw_u64 > static_cast<uint64_t>(INT32_MAX))
                                               ? INT32_MAX
                                               : static_cast<Units::RawFracQ16_16>(speed_raw_u64);
        Units::FracQ16_16 speed = Units::FracQ16_16::fromRaw(speed_raw);
        Units::AngleUnitsQ0_16 direction = atan2_turns_approx(clamp_raw_to_i16(y_raw),
                                                           clamp_raw_to_i16(x_raw));

        return LinearVector(x, y, speed, direction);
    }
}
