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

#include "renderer/pipeline/maths/units/Units.h"

namespace PolarShader {
    inline constexpr uint16_t MILLIS_PER_SECOND = 1000;

    /**
     * @brief Converts milliseconds to a f16/sf16 fixed-point representation of seconds.
     */
    sf16 timeMillisToScalar(TimeMillis millis);

    TimeMillis clampDeltaTime(TimeMillis deltaTime);
}

#endif // POLAR_SHADER_PIPELINE_MATHS_TIMEMATHS_H
