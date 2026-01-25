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
    struct CartQ24_8_Tag {
    };

    struct CartesianUQ24_8_Tag {
    };

    // Signed Cartesian coordinates in Q24.8; value represents Q0.16 lattice units with extra precision.
    using CartQ24_8 = Strong<int32_t, CartQ24_8_Tag>;

    // Unsigned Cartesian coordinates in Q24.8 for noise sampling.
    using CartUQ24_8 = Strong<uint32_t, CartesianUQ24_8_Tag>;

    constexpr int32_t raw(CartQ24_8 v) { return v.raw(); }
    constexpr uint32_t raw(CartUQ24_8 v) { return v.raw(); }
}

#endif // POLAR_SHADER_PIPELINE_UNITS_CARTESIANUNITS_H
