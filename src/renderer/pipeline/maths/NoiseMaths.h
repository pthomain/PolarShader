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

#ifndef POLAR_SHADER_PIPELINE_MATHS_NOISEMATHS_H
#define POLAR_SHADER_PIPELINE_MATHS_NOISEMATHS_H

#include "renderer/pipeline/maths/units/Units.h"
#include "renderer/pipeline/maths/PatternMaths.h"

namespace PolarShader {
    inline constexpr uint32_t NOISE_DOMAIN_OFFSET = 0x4000;
    inline constexpr uint16_t NOISE_MIN = 12000;
    inline constexpr uint16_t NOISE_MAX = 54000;

    /**
     * @brief Normalises a 16-bit raw noise value to the full 0-65535 range.
     */
    inline PatternNormU16 noiseNormaliseU16(NoiseRawU16 value) {
        return patternNormalize(raw(value), NOISE_MIN, NOISE_MAX);
    }

    /**
     * @brief Bilinear noise sampler for sr8/r8 coordinates using FastLED's inoise16.
     *
     * The inputs are unsigned r8 coordinates in noise domain space.
     */
    NoiseRawU16 sampleNoiseBilinear(uint32_t x, uint32_t y);

    NoiseRawU16 sampleNoiseTrilinear(uint32_t x, uint32_t y, uint32_t z);

    inline NoiseRawU16 sampleNoiseBilinear(r8 x, r8 y) {
        return sampleNoiseBilinear(raw(x), raw(y));
    }

    inline NoiseRawU16 sampleNoiseTrilinear(r8 x, r8 y, r8 z) {
        return sampleNoiseTrilinear(raw(x), raw(y), raw(z));
    }
}

#endif // POLAR_SHADER_PIPELINE_MATHS_NOISEMATHS_H
