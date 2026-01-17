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

#ifndef LED_SEGMENTS_PIPELINE_UTILS_TIMEUTILS_H
#define LED_SEGMENTS_PIPELINE_UTILS_TIMEUTILS_H

#include "polar/pipeline/utils/Units.h"

namespace LEDSegments {
    inline constexpr TimeMillis MAX_DELTA_TIME_MS = 200; // 0 disables delta-time clamping.

    inline TimeMillis clampDeltaTime(TimeMillis deltaTime) {
        if (MAX_DELTA_TIME_MS == 0) return deltaTime;
        return (deltaTime > MAX_DELTA_TIME_MS) ? MAX_DELTA_TIME_MS : deltaTime;
    }
}

#endif //LED_SEGMENTS_PIPELINE_UTILS_TIMEUTILS_H
