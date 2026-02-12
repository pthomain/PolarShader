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

#ifndef POLAR_SHADER_PIPELINE_MATHS_UVMATHS_H
#define POLAR_SHADER_PIPELINE_MATHS_UVMATHS_H

#include "units/Units.h"

namespace PolarShader {
    /**
     * @brief Provides basic arithmetic for unified UV coordinates.
     * 
     * These helpers ensure that coordinate offsets and transformations 
     * maintain precision across the Q16.16 domain.
     */
    namespace UVMaths {
        constexpr UV add(UV a, UV b) {
            return UV(
                SQ16_16(raw(a.u) + raw(b.u)),
                SQ16_16(raw(a.v) + raw(b.v))
            );
        }

        constexpr UV sub(UV a, UV b) {
            return UV(
                SQ16_16(raw(a.u) - raw(b.u)),
                SQ16_16(raw(a.v) - raw(b.v))
            );
        }
    }
}

#endif // POLAR_SHADER_PIPELINE_MATHS_UVMATHS_H
