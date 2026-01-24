//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

/*
 * This file is part of PolarShader.
 *
 * PolarShader is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PolarShader is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PolarShader. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef POLAR_SHADER_PIPELINE_CARTESIANNOISELAYERS_H
#define POLAR_SHADER_PIPELINE_CARTESIANNOISELAYERS_H

#include "FastLED.h"
#include "renderer/pipeline/maths/NoiseMaths.h"
#include "renderer/pipeline/units/Units.h"

namespace PolarShader {
    // All Cartesian noise layers return 16-bit intensities in [0..65535] suitable for palette mapping.
    // noiseLayer/fBm normalize raw inoise16(); turbulence/ridged derive palette-ready intensities directly.
    inline NoiseNormU16 noiseLayer(uint32_t x, uint32_t y) {
        // Normalize once at the source so downstream layers/palette mapping can assume 0..65535.
        return noiseNormaliseU16(NoiseRawU16(inoise16(x, y)));
    }

    inline NoiseNormU16 fBmLayer(uint32_t x, uint32_t y) {
        static fl::u8 octaves = 4;
        uint32_t r = 0;
        uint16_t amplitude = U16_HALF;
        for (int o = 0; o < octaves; o++) {
            NoiseNormU16 n = noiseNormaliseU16(NoiseRawU16(inoise16(x, y)));
            r += (static_cast<uint32_t>(raw(n)) * amplitude) >> 16;
            x <<= 1;
            y <<= 1;
            amplitude >>= 1;
        }
        if (r > UINT16_MAX) r = UINT16_MAX;
        return NoiseNormU16(static_cast<uint16_t>(r));
    }

    inline NoiseNormU16 turbulenceLayer(uint32_t x, uint32_t y) {
        NoiseRawU16 noise_raw = NoiseRawU16(inoise16(x, y));
        int16_t r = static_cast<int16_t>(raw(noise_raw)) - U16_HALF;
        uint16_t mag = static_cast<uint16_t>(r ^ (r >> 15)) - static_cast<uint16_t>(r >> 15);
        uint32_t doubled = static_cast<uint32_t>(mag) << 1;
        if (doubled > FRACT_Q0_16_MAX) doubled = FRACT_Q0_16_MAX;
        return NoiseNormU16(static_cast<uint16_t>(doubled));
    }

    inline NoiseNormU16 ridgedLayer(uint32_t x, uint32_t y) {
        NoiseRawU16 noise_raw = NoiseRawU16(inoise16(x, y));
        int16_t r = static_cast<int16_t>(raw(noise_raw)) - U16_HALF;
        uint16_t mag = static_cast<uint16_t>(r ^ (r >> 15)) - static_cast<uint16_t>(r >> 15);
        mag = min<uint16_t>(mag, static_cast<uint16_t>(U16_HALF - 1));
        uint32_t doubled = static_cast<uint32_t>(mag) << 1;
        if (doubled > FRACT_Q0_16_MAX) doubled = FRACT_Q0_16_MAX;
        uint16_t inverted = static_cast<uint16_t>(FRACT_Q0_16_MAX - doubled);
        return NoiseNormU16(inverted);
    }
}
#endif //POLAR_SHADER_PIPELINE_CARTESIANNOISELAYERS_H
