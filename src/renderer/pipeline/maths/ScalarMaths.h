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

#ifndef POLAR_SHADER_PIPELINE_MATHS_SCALARMATHS_H
#define POLAR_SHADER_PIPELINE_MATHS_SCALARMATHS_H

#include "FastLED.h"
#include "renderer/pipeline/units/AngleUnits.h"
#include "renderer/pipeline/units/ScalarUnits.h"
#include "renderer/pipeline/units/UnitConstants.h"

namespace PolarShader {
    fl::i32 scale32(fl::i32 value, FracQ0_16 scale);

    SFracQ0_16 mulSFracSat(SFracQ0_16 a, SFracQ0_16 b);

    SFracQ0_16 mulSFracWrap(SFracQ0_16 a, SFracQ0_16 b);

    uint64_t sqrtU64(uint64_t value);

    int32_t scaleSFracByTrig(SFracQ0_16 magnitude, TrigQ0_16 trig_q0_16);

    SFracQ0_16 scalarClampQ0_16Raw(int64_t raw_value);

    /**
     * @brief Create a Q0.16 bounded scalar from a rational fraction without floating point.
     */
    constexpr FracQ0_16 frac(uint32_t numerator, uint32_t denominator) {
        if (numerator == 0 || denominator == 0) return FracQ0_16(0);
        uint64_t raw_value = (static_cast<uint64_t>(FRACT_Q0_16_MAX) * numerator) / denominator;
        if (raw_value > FRACT_Q0_16_MAX) raw_value = FRACT_Q0_16_MAX;
        return FracQ0_16(static_cast<uint16_t>(raw_value));
    }

    constexpr FracQ0_16 frac(uint32_t denominator) {
        return frac(1u, denominator);
    }

    /**
     * @brief Create a signed Q0.16 scalar from a rational fraction without floating point.
     */
    constexpr SFracQ0_16 sFrac(uint32_t numerator, uint32_t denominator) {
        if (numerator == 0 || denominator == 0) return SFracQ0_16(0);
        uint64_t raw_value = (static_cast<uint64_t>(FRACT_Q0_16_MAX) * numerator) / denominator;
        if (raw_value > FRACT_Q0_16_MAX) raw_value = FRACT_Q0_16_MAX;
        return SFracQ0_16(static_cast<int32_t>(raw_value));
    }

    constexpr SFracQ0_16 sFrac(uint32_t denominator) {
        return sFrac(1u, denominator);
    }

    constexpr FracQ0_16 perMil(uint16_t perMil) {
        if (perMil == 0) return FracQ0_16(0);
        uint32_t raw_value = static_cast<uint32_t>((static_cast<uint64_t>(FRACT_Q0_16_MAX) * perMil) / 1000);
        return FracQ0_16(static_cast<uint16_t>(raw_value));
    }

    constexpr uint32_t clamp_frac_raw(int32_t raw_value) {
        if (raw_value <= 0) return 0u;
        if (raw_value >= static_cast<int32_t>(FRACT_Q0_16_MAX)) return FRACT_Q0_16_MAX;
        return static_cast<uint32_t>(raw_value);
    }
}

#endif // POLAR_SHADER_PIPELINE_MATHS_SCALARMATHS_H
