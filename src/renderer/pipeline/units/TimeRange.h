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

#ifndef POLAR_SHADER_PIPELINE_UNITS_TIME_RANGE_H
#define POLAR_SHADER_PIPELINE_UNITS_TIME_RANGE_H

#include "renderer/pipeline/units/ScalarUnits.h"
#include "renderer/pipeline/units/StrongTypes.h"
#include "renderer/pipeline/units/TimeUnits.h"

namespace PolarShader {
    /**
     * @brief Maps signals to time durations (TimeMillis).
     *
     * Clamps negative signal values to 0, as negative time durations are invalid in this context.
     */
    class TimeRange {
    public:
        TimeRange(TimeMillis durationMs = 30000);

        /**
         * @brief Maps a signed signal value [-1, 1] to a time duration.
         * @param t The input signal value.
         * @return A MappedSignal containing the resulting time duration.
         */
        MappedSignal<TimeMillis> map(SFracQ0_16 t) const;

    private:
        TimeMillis duration_ms = 0;
    };
}

#endif // POLAR_SHADER_PIPELINE_UNITS_TIME_RANGE_H
