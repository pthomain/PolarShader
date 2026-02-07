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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_INTERPOLATORS_INTERPOLATOR_H
#define POLAR_SHADER_PIPELINE_SIGNALS_INTERPOLATORS_INTERPOLATOR_H

#include "renderer/pipeline/maths/units/Units.h"

namespace PolarShader {
    /**
     * @brief Base class for all progress interpolators (easing functions).
     */
    class Interpolator {
    public:
        virtual ~Interpolator() = default;

        /**
         * @brief Maps a linear 0..1 progress value (Q0.16) to an interpolated 0..1 value (Q0.16).
         */
        virtual FracQ0_16 calculate(FracQ0_16 progress) const = 0;

        /**
         * @brief Clones the interpolator for ownership in signals.
         */
        virtual Interpolator* clone() const = 0;
    };
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_INTERPOLATORS_INTERPOLATOR_H
