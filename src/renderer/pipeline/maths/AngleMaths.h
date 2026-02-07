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

#ifndef POLAR_SHADER_PIPELINE_MATHS_ANGLEMATHS_H

#define POLAR_SHADER_PIPELINE_MATHS_ANGLEMATHS_H



#include "renderer/pipeline/units/Units.h"



namespace PolarShader {


    SFracQ0_16 angleSinQ0_16(FracQ0_16 a);

    SFracQ0_16 angleCosQ0_16(FracQ0_16 a);

    FracQ0_16 angleAtan2TurnsApprox(int16_t y, int16_t x);

    constexpr FracQ0_16 angleFrac(uint32_t denominator) {
        if (denominator == 0) return FracQ0_16(0);
        return FracQ0_16(static_cast<uint32_t>((static_cast<uint64_t>(ANGLE_FULL_TURN_U32)) / denominator));
    }

    constexpr FracQ0_16 angleFrac360(uint16_t angleDegrees) {
        if (angleDegrees == 0) return FracQ0_16(0);
        return FracQ0_16(static_cast<uint32_t>((static_cast<uint64_t>(FRACT_Q0_16_MAX) * angleDegrees) / 360));
    }

    // FastLED trig sampling helpers (explicit angle/phase conversions).
    constexpr uint16_t angleToFastLedPhase(FracQ0_16 a) { return raw(a); }

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

#endif // POLAR_SHADER_PIPELINE_MATHS_ANGLEMATHS_H
