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

#ifndef POLAR_SHADER_PIPELINE_RANGES_PALETTE_RANGE_H
#define POLAR_SHADER_PIPELINE_RANGES_PALETTE_RANGE_H

#include <cstdint>
#include "renderer/pipeline/ranges/Range.h"

namespace PolarShader {
    /**
     * @brief Maps normalized 0..1 signals into a palette index range [min, max].
     */
    class PaletteRange : public Range<PaletteRange, uint8_t> {
    public:
        PaletteRange(uint8_t minValue = 0, uint8_t maxValue = UINT8_MAX);

        MappedValue<uint8_t> map(SFracQ0_16 t) const override;

    private:
        uint8_t min_value;
        uint8_t max_value;
    };
}

#endif // POLAR_SHADER_PIPELINE_RANGES_PALETTE_RANGE_H
