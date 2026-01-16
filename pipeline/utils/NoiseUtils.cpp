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

#include "NoiseUtils.h"

namespace LEDSegments {
    Units::NoiseNormU16 normaliseNoise16(Units::NoiseRawU16 value) {
        // These bounds are specific to the typical output of inoise16() and may
        // need tuning if a different noise function is used.
        const uint16_t MIN_NOISE = 12000;
        const uint16_t MAX_NOISE = 54000;
        const uint16_t RANGE = MAX_NOISE - MIN_NOISE;

        if (value <= MIN_NOISE) return 0;
        if (value >= MAX_NOISE) return Units::FRACT_Q0_16_MAX;

        uint32_t temp = (uint32_t) (value - MIN_NOISE) * Units::FRACT_Q0_16_MAX;
        return static_cast<Units::NoiseNormU16>(temp / RANGE);
    }
}
