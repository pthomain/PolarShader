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

#include "renderer/pipeline/units/AngleUnits.h"
#include "renderer/pipeline/units/UnitConstants.h"

namespace PolarShader {
    TrigQ0_16 angleSinQ0_16(AngleQ0_16 a);

    TrigQ0_16 angleCosQ0_16(AngleQ0_16 a);

    AngleQ0_16 angleAtan2TurnsApprox(int16_t y, int16_t x);

    constexpr AngleQ0_16 angleFrac(uint32_t denominator) {
        if (denominator == 0) return AngleQ0_16(0);
        return AngleQ0_16(static_cast<uint32_t>((static_cast<uint64_t>(ANGLE_FULL_TURN_U32)) / denominator));
    }

    constexpr AngleQ0_16 angleFrac360(uint16_t angleDegrees) {
        if (angleDegrees == 0) return AngleQ0_16(0);
        return AngleQ0_16(static_cast<uint32_t>((static_cast<uint64_t>(FRACT_Q0_16_MAX) * angleDegrees) / 360));
    }

    // FastLED trig sampling helpers (explicit angle/phase conversions).
    constexpr uint16_t angleToFastLedPhase(AngleQ0_16 a) { return raw(a); }
}

#endif // POLAR_SHADER_PIPELINE_MATHS_ANGLEMATHS_H
