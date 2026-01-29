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

#ifndef POLAR_SHADER_PIPELINE_RANGES_DEPTH_RANGE_H
#define POLAR_SHADER_PIPELINE_RANGES_DEPTH_RANGE_H

#include "renderer/pipeline/units/ScalarUnits.h"
#include "renderer/pipeline/units/StrongTypes.h"

namespace PolarShader {
    /**
     * @brief Maps a 0..1 signal into the unsigned Q24.8 depth domain.
     */
    class DepthRange {
    public:
        DepthRange(uint32_t min = 0u, uint32_t max = UINT32_MAX >> 4);

        /**
         * @brief Maps a signed signal value [0, 1] to a depth in the specified range.
         * @param t The input signal value.
         * @return A MappedSignal containing the resulting depth value.
         */
        MappedSignal<uint32_t> map(SFracQ0_16 t) const;

    private:
        uint32_t min_depth = 0u;
        uint32_t max_depth = 0u;
    };
}

#endif // POLAR_SHADER_PIPELINE_RANGES_DEPTH_RANGE_H
