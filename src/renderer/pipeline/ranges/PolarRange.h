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

#ifndef POLAR_SHADER_PIPELINE_UNITS_POLAR_RANGE_H
#define POLAR_SHADER_PIPELINE_UNITS_POLAR_RANGE_H

#include "renderer/pipeline/ranges/Range.h"
#include "renderer/pipeline/units/ScalarUnits.h"
#include "renderer/pipeline/units/UnitConstants.h"

namespace PolarShader {
    /**
     * @brief Maps signals to angular values (FracQ0_16).
     *
     * Handles wrapping logic appropriate for angles (e.g., 0-360 degrees).
     */
    class PolarRange : public Range<PolarRange, FracQ0_16> {
    public:
        PolarRange(FracQ0_16 min = FracQ0_16(0), FracQ0_16 max = FracQ0_16(FRACT_Q0_16_MAX));

        /**
         * @brief Maps a signed signal value [-1, 1] to an angle in the specified range.
         * @param t The input signal value.
         * @return A MappedValue containing the resulting angle.
         */
        MappedValue<FracQ0_16> map(SFracQ0_16 t) const override;

    private:
        FracQ0_16 min_frac = FracQ0_16(0);
        FracQ0_16 max_frac = FracQ0_16(FRACT_Q0_16_MAX);
    };
}

#endif // POLAR_SHADER_PIPELINE_UNITS_POLAR_RANGE_H
