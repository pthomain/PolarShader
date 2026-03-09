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

#ifndef POLAR_SHADER_PIPELINE_MATHS_CARTESIANMATHS_H
#define POLAR_SHADER_PIPELINE_MATHS_CARTESIANMATHS_H

#include "renderer/pipeline/maths/units/Units.h"

namespace PolarShader {
    namespace CartesianMaths {
        /** @brief Converts UV coordinate (Q16.16) to Cartesian (Q24.8) by dropping 8 fractional bits. */
        constexpr fl::s24x8 from_uv(fl::s16x16 uv_coord) {
            return fl::s24x8::from_raw(uv_coord.raw() >> 8);
        }

        /** @brief Converts Cartesian coordinate (Q24.8) back to UV (Q16.16) by adding 8 fractional bits. */
        constexpr fl::s16x16 to_uv(fl::s24x8 cart) {
            return fl::s16x16::from_raw(cart.raw() << 8);
        }
    }
}

#endif // POLAR_SHADER_PIPELINE_MATHS_CARTESIANMATHS_H
