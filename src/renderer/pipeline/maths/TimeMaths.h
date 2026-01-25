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

#ifndef POLAR_SHADER_PIPELINE_MATHS_TIMEMATHS_H
#define POLAR_SHADER_PIPELINE_MATHS_TIMEMATHS_H

#include "renderer/pipeline/units/ScalarUnits.h"
#include "renderer/pipeline/units/TimeUnits.h"

namespace PolarShader {
    inline constexpr uint16_t MILLIS_PER_SECOND = 1000;

    /**
     * @brief Converts milliseconds to a Q0.16 fixed-point representation of seconds.
     */
    inline SFracQ0_16 timeMillisToScalar(TimeMillis millis) {
        int64_t dt_raw = (static_cast<int64_t>(millis) << 16) + (MILLIS_PER_SECOND / 2);
        dt_raw /= MILLIS_PER_SECOND;
        return SFracQ0_16(static_cast<int32_t>(dt_raw));
    }
}

#endif // POLAR_SHADER_PIPELINE_MATHS_TIMEMATHS_H
