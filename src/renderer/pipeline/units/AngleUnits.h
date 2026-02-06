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

#ifndef POLAR_SHADER_PIPELINE_UNITS_ANGLEUNITS_H
#define POLAR_SHADER_PIPELINE_UNITS_ANGLEUNITS_H

#include "renderer/pipeline/units/ScalarUnits.h"
#include "renderer/pipeline/units/UnitConstants.h"

namespace PolarShader {
    /**
     * @brief Helpers for angular arithmetic in the 16-bit turn domain.
     * 
     * Analysis: Angles are represented by FracQ0_16 (unsigned 16-bit turns).
     * This type is strictly required because its natural overflow behavior (mod 65536) 
     * perfectly models circular rotation without the need for manual modulo logic.
     * 
     * Usage: Polar coordinate Angle components, Rotation transforms, and oscillators.
     */

    // --- Angle wrap arithmetic (mod 2^16) ---
    constexpr FracQ0_16 angleWrapAdd(FracQ0_16 a, uint16_t delta) {
        return FracQ0_16(static_cast<uint16_t>(raw(a) + delta));
    }

    // Wrap-add using signed raw delta (Q0.16), interpreted via two's-complement.
    constexpr FracQ0_16 angleWrapAddSigned(FracQ0_16 a, int32_t delta_raw_q0_16) {
        uint32_t sum = static_cast<uint32_t>(raw(a)) + static_cast<uint32_t>(delta_raw_q0_16);
        return FracQ0_16(static_cast<uint16_t>(sum));
    }
}

#endif // POLAR_SHADER_PIPELINE_UNITS_ANGLEUNITS_H