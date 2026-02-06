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

#ifndef POLAR_SHADER_PIPELINE_UNITS_SCALARUNITS_H
#define POLAR_SHADER_PIPELINE_UNITS_SCALARUNITS_H

#include <cstdint>
#include "StrongTypes.h"

namespace PolarShader {
    struct FracQ0_16_Tag {};
    struct SFracQ0_16_Tag {};
    struct FracQ16_16_Tag {};

    /**
     * @brief Unsigned fixed-point fraction in Q0.16 format.
     * 
     * Definition: 16-bit integer where 0 represents 0.0 and 65535 represents ~1.0.
     * Usage: Used for angles (mod 2^16), alpha blending, and unsigned scaling factors.
     * Analysis: Strictly required for circular math and operations where negative values are semantically invalid.
     */
    using FracQ0_16 = Strong<uint16_t, FracQ0_16_Tag>;

    /**
     * @brief Signed fixed-point scalar in Q0.16 format stored in a 32-bit container.
     * 
     * Definition: Signed integer where 65536 represents 1.0 and -65536 represents -1.0.
     * Usage: The primary currency for signals, oscillators, and trigonometric outputs (sine/cosine).
     * Analysis: Strictly required for the signal engine to support bidirectional modulation (e.g. oscillating around zero).
     * Replaces legacy TrigQ0_16.
     */
    using SFracQ0_16 = Strong<int32_t, SFracQ0_16_Tag>;

    /**
     * @brief Signed fixed-point scalar in Q16.16 format.
     * 
     * Definition: 16 integer bits and 16 fractional bits. 65536 represents 1.0.
     * Usage: Used exclusively for UV spatial coordinates.
     * Analysis: Strictly required to provide enough dynamic range for tiling/zoom (values > 1.0) while maintaining sub-pixel precision.
     */
    using FracQ16_16 = Strong<int32_t, FracQ16_16_Tag>;

    // --- Raw extractors ---
    constexpr uint16_t raw(FracQ0_16 f) { return f.raw(); }
    constexpr int32_t raw(SFracQ0_16 v) { return v.raw(); }
    constexpr int32_t raw(FracQ16_16 v) { return v.raw(); }
}

#endif // POLAR_SHADER_PIPELINE_UNITS_SCALARUNITS_H