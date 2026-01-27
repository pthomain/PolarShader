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

#ifndef POLAR_SHADER_PIPELINE_UNITS_SCALAR_RANGE_H
#define POLAR_SHADER_PIPELINE_UNITS_SCALAR_RANGE_H

#include "renderer/pipeline/units/NoiseUnits.h"
#include "renderer/pipeline/units/ScalarUnits.h"
#include "renderer/pipeline/units/StrongTypes.h"

namespace PolarShader {
    /**
     * @brief Maps signals to scalar integer values (int32_t).
     *
     * Linearly interpolates the input signal [-1, 1] to the target range [min, max].
     */
    class ScalarRange {
    public:
        ScalarRange(int32_t min, int32_t max);

        /**
         * @brief Maps a signed signal value [-1, 1] to a scalar in the specified range.
         * @param t The input signal value.
         * @return A MappedSignal containing the resulting scalar.
         */
        MappedSignal<int32_t> map(SFracQ0_16 t) const;

    private:
        int32_t min_scalar = 0;
        int32_t max_scalar = 0;
    };
}

#endif // POLAR_SHADER_PIPELINE_UNITS_SCALAR_RANGE_H
