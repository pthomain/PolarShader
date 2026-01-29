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

#ifndef POLAR_SHADER_PIPELINE_RANGES_CART_RANGE_H
#define POLAR_SHADER_PIPELINE_RANGES_CART_RANGE_H

#include "renderer/pipeline/ranges/Range.h"
#include "renderer/pipeline/units/UnitConstants.h"
#include "renderer/pipeline/units/CartesianUnits.h"

namespace PolarShader {
    /**
     * @brief Maps normalized 0..1 signals into CartQ24_8 values.
     */
    class CartRange : public Range<CartRange, CartQ24_8> {
    public:
        CartRange(CartQ24_8 minValue = CartQ24_8(0), CartQ24_8 maxValue = CartQ24_8(1 << CARTESIAN_FRAC_BITS));

        MappedValue<CartQ24_8> map(SFracQ0_16 t) const override;

    private:
        int32_t min_raw = 0;
        int32_t max_raw = 0;
    };
}

#endif // POLAR_SHADER_PIPELINE_RANGES_CART_RANGE_H
