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

#ifndef LED_SEGMENTS_EFFECTS_MAPPERS_SIGNALPOLICIES_H
#define LED_SEGMENTS_EFFECTS_MAPPERS_SIGNALPOLICIES_H

namespace LEDSegments {
    struct LinearPolicy {
        void apply(int32_t &, int32_t &) const {}
    };

    struct ClampPolicy {
        using Q16_16 = int32_t;
        Q16_16 min_val;
        Q16_16 max_val;

        ClampPolicy(int32_t min, int32_t max) : min_val(min << 16), max_val(max << 16) {}

        void apply(Q16_16 &position, Q16_16 &velocity) const {
            if (position < min_val) {
                position = min_val;
                velocity = 0;
            } else if (position > max_val) {
                position = max_val;
                velocity = 0;
            }
        }
    };

    struct WrapPolicy {
        using Q16_16 = int32_t;
        // Wrap value in integer units (not Q16.16) to avoid 32-bit overflow of 65536<<16
        int32_t wrap_units;

        explicit WrapPolicy(int32_t wrap = 65536) : wrap_units(wrap) {}

        void apply(Q16_16 &position, Q16_16 &) const {
            if (wrap_units <= 0) return;
            // Promote to 64-bit to perform modulo by (wrap_units << 16) safely
            int64_t pos64 = static_cast<int64_t>(position);
            const int64_t wrap_q16_16 = (static_cast<int64_t>(wrap_units) << 16);
            if (pos64 >= wrap_q16_16 || pos64 < 0) {
                pos64 %= wrap_q16_16;
                if (pos64 < 0) pos64 += wrap_q16_16;
                position = static_cast<Q16_16>(pos64);
            }
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_MAPPERS_SIGNALPOLICIES_H
