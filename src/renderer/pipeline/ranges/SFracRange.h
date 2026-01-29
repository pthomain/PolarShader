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

#ifndef POLAR_SHADER_PIPELINE_RANGES_SFRACRANGE_H
#define POLAR_SHADER_PIPELINE_RANGES_SFRACRANGE_H

#include "renderer/pipeline/ranges/Range.h"
#include "renderer/pipeline/units/ScalarUnits.h"
#include "renderer/pipeline/units/UnitConstants.h"

namespace PolarShader {
    /**
     * @brief Maps a 0..1 signal into a signed Q0.16 range.
     */
    class SFracRange : public Range<SFracRange, SFracQ0_16> {
    public:
        SFracRange(SFracQ0_16 minValue = SFracQ0_16(0), SFracQ0_16 maxValue = SFracQ0_16(Q0_16_ONE));

        MappedValue<SFracQ0_16> map(SFracQ0_16 t) const override;

    private:
        int32_t min_raw = 0;
        int32_t max_raw = 0;
    };
}

#endif // POLAR_SHADER_PIPELINE_RANGES_SFRACRANGE_H
