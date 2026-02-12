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
     * @brief Provides explicit, safe helpers for fixed-point `sr8` arithmetic.
     *
     * Using these helpers is crucial for avoiding common fixed-point errors, such as
     * incorrect scaling or precision loss, especially in grid-based patterns.
     */
    namespace CartesianMaths {
        /** @brief Converts an integer into an `sr8` fixed-point value. */
        constexpr sr8 from_int(int32_t i) {
            return sr8(i << R8_FRAC_BITS);
        }

        /** @brief Converts UV to `sr8` Cartesian, typically for sampling. */
        constexpr sr8 from_uv(sr16 uv_coord) {
            // UV is r16/sr16 (Q16.16), Cartesian intermediate is sr8 (Q24.8).
            // We shift right by 8 to convert from 16 fractional bits to 8.
            return sr8(raw(uv_coord) >> 8);
        }

        /** @brief Converts `sr8` Cartesian back to UV. */
        constexpr sr16 to_uv(sr8 cart) {
            // Cartesian intermediate is sr8 (Q24.8), UV is r16/sr16 (Q16.16).
            // We shift left by 8 to convert from 8 fractional bits to 16.
            return sr16(raw(cart) << 8);
        }

        /** @brief Gets the integer part of an `sr8` value, effectively flooring it. */
        constexpr int32_t floor_to_int(sr8 q) {
            return raw(q) >> R8_FRAC_BITS;
        }

        /** @brief Extracts the fractional part of an `sr8` value. */
        constexpr sr8 fract(sr8 q) {
            constexpr int32_t frac_mask = (1 << R8_FRAC_BITS) - 1;
            return sr8(raw(q) & frac_mask);
        }

        /** @brief Multiplies two `sr8` values. */
        constexpr sr8 mul(sr8 a, sr8 b) {
            int64_t temp = static_cast<int64_t>(raw(a)) * static_cast<int64_t>(raw(b));
            return sr8(static_cast<int32_t>(temp >> R8_FRAC_BITS));
        }

        /** @brief Divides two `sr8` values. */
        constexpr sr8 div(sr8 a, sr8 b) {
            if (raw(b) == 0) return sr8(INT32_MAX); // Should be handled by caller
            int64_t temp = static_cast<int64_t>(raw(a)) << R8_FRAC_BITS;
            return sr8(static_cast<int32_t>(temp / raw(b)));
        }
    } // namespace CartesianMaths
} // namespace PolarShader

#endif // POLAR_SHADER_PIPELINE_MATHS_CARTESIANMATHS_H
