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

#ifndef LED_SEGMENTS_PIPELINE_CARTESIANNOISELAYERS_H
#define LED_SEGMENTS_PIPELINE_CARTESIANNOISELAYERS_H

#include "utils/NoiseUtils.h"
#include "utils/Units.h"

namespace LEDSegments {
    // All Cartesian noise layers return 16-bit intensities in [0..65535] suitable for palette mapping.
    // noiseLayer/fBm normalize raw inoise16(); turbulence/ridged derive palette-ready intensities directly.
    inline Units::NoiseNormU16 noiseLayer(uint32_t x, uint32_t y) {
        // Normalize once at the source so downstream layers/palette mapping can assume 0..65535.
        return normaliseNoise16(inoise16(x, y));
    }

    inline Units::NoiseNormU16 fBmLayer(uint32_t x, uint32_t y) {
        static fl::u8 octaves = 4;
        uint32_t r = 0;
        uint16_t amplitude = Units::U16_HALF;
        for (int o = 0; o < octaves; o++) {
            Units::NoiseNormU16 n = normaliseNoise16(inoise16(x, y));
            r += (n * amplitude) >> 16;
            x <<= 1;
            y <<= 1;
            amplitude >>= 1;
        }
        if (r > UINT16_MAX) r = UINT16_MAX;
        return static_cast<Units::NoiseNormU16>(r);
    }

    inline Units::NoiseNormU16 turbulenceLayer(uint32_t x, uint32_t y) {
        int16_t r = (int16_t) inoise16(x, y) - Units::U16_HALF;
        uint16_t mag = (uint16_t) (r ^ (r >> 15)) - (uint16_t) (r >> 15);
        uint32_t doubled = static_cast<uint32_t>(mag) << 1;
        if (doubled > Units::FRACT_Q0_16_MAX) doubled = Units::FRACT_Q0_16_MAX;
        return static_cast<Units::NoiseNormU16>(doubled);
    }

    inline Units::NoiseNormU16 ridgedLayer(uint32_t x, uint32_t y) {
        int16_t r = (int16_t) inoise16(x, y) - Units::U16_HALF;
        uint16_t mag = (uint16_t) (r ^ (r >> 15)) - (uint16_t) (r >> 15);
        mag = min<uint16_t>(mag, static_cast<uint16_t>(Units::TRIG_Q1_15_MAX));
        uint32_t doubled = static_cast<uint32_t>(mag) << 1;
        if (doubled > Units::FRACT_Q0_16_MAX) doubled = Units::FRACT_Q0_16_MAX;
        uint16_t inverted = static_cast<uint16_t>(Units::FRACT_Q0_16_MAX - doubled);
        return inverted;
    }
}
#endif //LED_SEGMENTS_PIPELINE_CARTESIANNOISELAYERS_H
