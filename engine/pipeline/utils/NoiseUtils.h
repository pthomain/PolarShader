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

#ifndef LED_SEGMENTS_SPECS_NOISEUTILS_H
#define LED_SEGMENTS_SPECS_NOISEUTILS_H

#include "FastLED.h"

namespace LEDSegments {
    static constexpr uint32_t NOISE_DOMAIN_OFFSET = 0x800000;

    /**
        * @brief Normalizes a 16-bit raw noise value to the full 0-65535 range.
        *
        * This function remaps the typical output range of FastLED's inoise16()
        * (empirically found to be around 12000-54000) to the full 16-bit range.
        * This increases the contrast of the noise, which is useful for visual effects.
        *
        * @param value The raw 16-bit noise value.
        * @return The normalized 16-bit value.
        */
    static fl::u16 normaliseNoise16(fl::u16 value) {
        // These bounds are specific to the typical output of inoise16() and may
        // need tuning if a different noise function is used.
        const uint16_t MIN_NOISE = 12000;
        const uint16_t MAX_NOISE = 54000;
        const uint16_t RANGE = MAX_NOISE - MIN_NOISE;

        if (value <= MIN_NOISE) return 0;
        if (value >= MAX_NOISE) return 65535;

        uint32_t temp = (uint32_t) (value - MIN_NOISE) * 65535;
        return temp / RANGE;
    }
}

#endif //LED_SEGMENTS_SPECS_NOISEUTILS_H
