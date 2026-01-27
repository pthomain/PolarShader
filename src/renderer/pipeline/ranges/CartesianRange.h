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

#ifndef POLAR_SHADER_PIPELINE_UNITS_CARTESIAN_RANGE_H
#define POLAR_SHADER_PIPELINE_UNITS_CARTESIAN_RANGE_H

#include "renderer/pipeline/units/ScalarUnits.h"
#include "renderer/pipeline/units/StrongTypes.h"

namespace PolarShader {
    struct SPoint32 {
        int32_t x;
        int32_t y;
    };

    /**
     * @brief Maps direction and velocity signals to 2D Cartesian coordinates (SPoint32).
     *
     * Converts polar inputs (direction, velocity) into Cartesian (x, y) offsets based on a radius.
     */
    class CartesianRange {
    public:
        CartesianRange(int32_t radius = UINT32_MAX >> 5);

        /**
         * @brief Maps direction and velocity signals to a Cartesian vector.
         * @param direction The direction signal value.
         * @param velocity The velocity signal value.
         * @return A MappedSignal containing the resulting Cartesian vector.
         */
        MappedSignal<SPoint32> map(SFracQ0_16 direction, SFracQ0_16 velocity) const;

    private:
        int32_t radius = 0;
    };
}

#endif // POLAR_SHADER_PIPELINE_UNITS_CARTESIAN_RANGE_H
