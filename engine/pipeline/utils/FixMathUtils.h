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
#pragma once

#ifndef LED_SEGMENTS_PIPELINE_UTILS_FIXMATHUTILS_H
#define LED_SEGMENTS_PIPELINE_UTILS_FIXMATHUTILS_H

#include "Units.h"

namespace LEDSegments {

    inline constexpr uint16_t MILLIS_PER_SECOND = 1000;

    /**
     * @brief Converts milliseconds to a Q16.16 fixed-point representation of seconds.
     *
     * This function converts a duration in milliseconds to a proportion of a second,
     * represented as a Q16.16 value. For example, 500ms would be converted to 0.5
     * in Q16.16 format.
     *
     * The calculation is performed using 64-bit intermediates to maintain precision
     * and avoid overflow, with proper rounding.
     *
     * @param millis The time in milliseconds.
     * @return The equivalent time in seconds as a SignalQ16_16 value.
     */
    inline Units::SignalQ16_16 millisToQ16_16(Units::TimeMillis millis) {
        // (millis / 1000) * 2^16
        // To maintain precision, this is calculated as:
        // (millis * 2^16) / 1000
        // with rounding.
        int64_t dt_raw = (static_cast<int64_t>(millis) << 16) + (MILLIS_PER_SECOND / 2);
        dt_raw /= MILLIS_PER_SECOND;
        return Units::SignalQ16_16::fromRaw(static_cast<Units::RawSignalQ16_16>(dt_raw));
    }
}

#endif //LED_SEGMENTS_PIPELINE_UTILS_FIXMATHUTILS_H
