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

#ifndef POLAR_SHADER_PIPELINE_UNITS_TIMEUNITS_H
#define POLAR_SHADER_PIPELINE_UNITS_TIMEUNITS_H

namespace PolarShader {
    // Time
    using TimeMillis = unsigned long; // Arduino millis()

    inline constexpr TimeMillis MAX_DELTA_TIME_MS = 200; // 0 disables delta-time clamping.

    inline TimeMillis clampDeltaTime(TimeMillis deltaTime) {
        if (MAX_DELTA_TIME_MS == 0) return deltaTime;
        return (deltaTime > MAX_DELTA_TIME_MS) ? MAX_DELTA_TIME_MS : deltaTime;
    }
}

#endif // POLAR_SHADER_PIPELINE_UNITS_TIMEUNITS_H
