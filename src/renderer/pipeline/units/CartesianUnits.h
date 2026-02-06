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

#ifndef POLAR_SHADER_PIPELINE_UNITS_CARTESIANUNITS_H
#define POLAR_SHADER_PIPELINE_UNITS_CARTESIANUNITS_H

#include <cstdint>
#include "StrongTypes.h"

namespace PolarShader {
    /**
     * @brief A simple 2D signed integer vector.
     * 
     * Usage: Lightweight tuple for raw coordinate pairs or intermediate displacements.
     * Analysis: Useful internal utility, distinct from the unified UV coordinate system.
     */
    struct SPoint32 {
        int32_t x;
        int32_t y;
    };

    struct CartQ24_8_Tag {};
    struct CartesianUQ24_8_Tag {};

    /**
     * @brief Signed fixed-point coordinate in Q24.8 format.
     * 
     * Definition: 24 integer bits, 8 fractional bits. 256 represents 1.0.
     * Usage: Implementation detail for lattice-aligned patterns (Worley, HexTiling).
     * Analysis: Strictly required for algorithms that align with integer grid indices while needing some sub-unit precision.
     */
    using CartQ24_8 = Strong<int32_t, CartQ24_8_Tag>;

    /**
     * @brief Unsigned fixed-point coordinate in Q24.8 format.
     * 
     * Usage: Exclusively for noise domain sampling (inoise16).
     * Analysis: Required to match the unsigned interface of the hardware-optimized noise generators.
     */
    using CartUQ24_8 = Strong<uint32_t, CartesianUQ24_8_Tag>;

    constexpr int32_t raw(CartQ24_8 v) { return v.raw(); }
    constexpr uint32_t raw(CartUQ24_8 v) { return v.raw(); }
}

#endif // POLAR_SHADER_PIPELINE_UNITS_CARTESIANUNITS_H