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

#include "../utils/FixMathUtils.h"
#include "../utils/Units.h"

namespace LEDSegments {

    struct LinearPolicy {
        void apply(Units::SignalQ16_16 &, Units::SignalQ16_16 &) const {}
    };

    struct ClampPolicy {
        Units::SignalQ16_16 min_val;
        Units::SignalQ16_16 max_val;

        ClampPolicy(int32_t min, int32_t max);

        void apply(Units::SignalQ16_16 &position, Units::SignalQ16_16 &velocity) const;
    };

    struct WrapPolicy {
        int32_t wrap_units;

        explicit WrapPolicy(int32_t wrap = 65536);

        void apply(Units::SignalQ16_16 &position, Units::SignalQ16_16 &) const;
    };
}

#endif //LED_SEGMENTS_EFFECTS_MAPPERS_SIGNALPOLICIES_H
