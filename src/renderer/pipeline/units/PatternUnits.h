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

#ifndef POLAR_SHADER_PIPELINE_UNITS_PATTERNUNITS_H
#define POLAR_SHADER_PIPELINE_UNITS_PATTERNUNITS_H

#include <cstdint>
#include "StrongTypes.h"

namespace PolarShader {
    struct NoiseRawU16_Tag {};
    struct PatternNormU16_Tag {};

    /**
     * @brief Raw 16-bit output from a noise generator.
     * 
     * Usage: Transient type in NoiseMaths before normalization.
     * Analysis: Required to distinguish "raw" unmapped data from "clean" pipeline data.
     */
    using NoiseRawU16 = Strong<uint16_t, NoiseRawU16_Tag>;

    /**
     * @brief Strictly normalized 16-bit pattern intensity.
     * 
     * Definition: Unsigned 16-bit integer spanning the full 0..65535 range.
     * Usage: The standard output of every UVPattern and the standard input for palette mapping.
     * Analysis: Strictly required as the "universal currency" of the visual pipeline.
     */
    using PatternNormU16 = Strong<uint16_t, PatternNormU16_Tag>;

    // --- Raw extractors ---
    constexpr uint16_t raw(NoiseRawU16 n) { return n.raw(); }
    constexpr uint16_t raw(PatternNormU16 n) { return n.raw(); }
}

#endif // POLAR_SHADER_PIPELINE_UNITS_PATTERNUNITS_H