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

#include "SignalPolicies.h"
#include <cstring>

namespace LEDSegments {

    ClampPolicy::ClampPolicy() : min_val(INT32_MIN), max_val(INT32_MAX) {}

    ClampPolicy::ClampPolicy(int32_t min, int32_t max) : min_val(min), max_val(max) {}

    ClampPolicy::ClampPolicy(Units::SignalQ16_16 min, Units::SignalQ16_16 max) : min_val(min), max_val(max) {}

    void ClampPolicy::apply(Units::SignalQ16_16 &position, Units::SignalQ16_16 &velocity) const {
        if (position < min_val) {
            position = min_val;
            velocity = Units::SignalQ16_16(0);
        } else if (position > max_val) {
            position = max_val;
            velocity = Units::SignalQ16_16(0);
        }
    }

    WrapPolicy::WrapPolicy(int32_t wrap) : wrap_units(wrap) {}

    void WrapPolicy::apply(Units::SignalQ16_16 &position, Units::SignalQ16_16 &) const {
        if (wrap_units <= 0) return;

        const int64_t wrap_q16_16 = static_cast<int64_t>(wrap_units) << 16;
        int64_t pos64 = position.asRaw();

        if (pos64 >= wrap_q16_16 || pos64 < 0) {
            pos64 %= wrap_q16_16;
            if (pos64 < 0) {
                pos64 += wrap_q16_16;
            }
        }
        position = Units::SignalQ16_16::fromRaw(static_cast<int32_t>(pos64));
    }
}
