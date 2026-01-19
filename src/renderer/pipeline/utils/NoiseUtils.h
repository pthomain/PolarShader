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

#ifndef POLAR_SHADER_PIPELINE_UTILS_NOISEUTILS_H
#define POLAR_SHADER_PIPELINE_UTILS_NOISEUTILS_H

#include "Units.h"

namespace PolarShader {
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
    NoiseNormU16 normaliseNoise16(NoiseRawU16 value);
}

#endif //POLAR_SHADER_PIPELINE_UTILS_NOISEUTILS_H
