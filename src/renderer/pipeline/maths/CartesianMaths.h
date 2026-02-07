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

#ifndef POLAR_SHADER_PIPELINE_MATHS_CARTESIANMATHS_H
#define POLAR_SHADER_PIPELINE_MATHS_CARTESIANMATHS_H

#include "renderer/pipeline/maths/units/Units.h"

namespace PolarShader {
    /**
     * @brief Provides explicit, safe helpers for fixed-point (Q24.8) arithmetic.
     *
     * Using these helpers is crucial for avoiding common fixed-point errors, such as
     * incorrect scaling or precision loss, especially in grid-based patterns.
     */
    namespace CartesianMaths {
        /** @brief Converts an integer into a Q24.8 fixed-point value. */
        constexpr CartQ24_8 from_int(int32_t i) {
            return CartQ24_8(i << CARTESIAN_FRAC_BITS);
        }

        /** @brief Converts UV to CartQ24.8, typically for sampling. */
        constexpr CartQ24_8 from_uv(FracQ16_16 uv_coord) {
            // UV is Q16.16, CartQ24.8 is Q24.8.
            // We shift right by 8 to convert from 16 fractional bits to 8.
            return CartQ24_8(raw(uv_coord) >> 8);
        }

        /** @brief Converts CartQ24.8 back to UV. */
        constexpr FracQ16_16 to_uv(CartQ24_8 cart) {
            // CartQ24.8 is Q24.8, UV is Q16.16.
            // We shift left by 8 to convert from 8 fractional bits to 16.
            return FracQ16_16(raw(cart) << 8);
        }

        /** @brief Gets the integer part of a Q24.8 value, effectively flooring it. */
        constexpr int32_t floor_to_int(CartQ24_8 q) {
            return raw(q) >> CARTESIAN_FRAC_BITS;
        }

        /** @brief Extracts the fractional part of a Q24.8 value. */
        constexpr CartQ24_8 fract(CartQ24_8 q) {
            constexpr int32_t frac_mask = (1 << CARTESIAN_FRAC_BITS) - 1;
            return CartQ24_8(raw(q) & frac_mask);
        }

        /** @brief Multiplies two Q24.8 values. */
        constexpr CartQ24_8 mul(CartQ24_8 a, CartQ24_8 b) {
            int64_t temp = static_cast<int64_t>(raw(a)) * static_cast<int64_t>(raw(b));
            return CartQ24_8(static_cast<int32_t>(temp >> CARTESIAN_FRAC_BITS));
        }

        /** @brief Divides two Q24.8 values. */
        constexpr CartQ24_8 div(CartQ24_8 a, CartQ24_8 b) {
            if (raw(b) == 0) return CartQ24_8(INT32_MAX); // Should be handled by caller
            int64_t temp = static_cast<int64_t>(raw(a)) << CARTESIAN_FRAC_BITS;
            return CartQ24_8(static_cast<int32_t>(temp / raw(b)));
        }
    } // namespace CartesianMaths
} // namespace PolarShader

#endif // POLAR_SHADER_PIPELINE_MATHS_CARTESIANMATHS_H
