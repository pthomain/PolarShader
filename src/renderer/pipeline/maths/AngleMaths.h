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
    TrigQ1_15 angleSinQ1_15(BoundedAngle a);

    TrigQ1_15 angleSinQ1_15(UnboundedAngle p);

    TrigQ1_15 angleCosQ1_15(BoundedAngle a);

    TrigQ1_15 angleCosQ1_15(UnboundedAngle p);

    BoundedAngle angleAtan2TurnsApprox(int16_t y, int16_t x);

    /**
     * @brief Create a bounded angle from a rational fraction without floating point.
     */
    constexpr BoundedAngle angleFromRatioBounded(uint32_t num, uint32_t den) {
        if (den == 0) return BoundedAngle(0);
        uint32_t raw_value = static_cast<uint32_t>((static_cast<uint64_t>(num) * FRACT_Q0_16_MAX) / den);
        return BoundedAngle(static_cast<uint16_t>(raw_value));
    }

    /**
     * @brief Create an unbounded angle (Q16.16 turns) from a rational fraction without floating point.
     */
    constexpr BoundedAngle angleFrac(uint32_t denominator) {
        if (denominator == 0) return BoundedAngle(0);
        return BoundedAngle(static_cast<uint32_t>((static_cast<uint64_t>(1) << 16) / denominator));
    }

    constexpr BoundedAngle angleFrac360(uint16_t angleDegrees) {
        if (angleDegrees == 0) return BoundedAngle(0);
        return BoundedAngle(static_cast<uint32_t>((static_cast<uint64_t>(FRACT_Q0_16_MAX) * angleDegrees) / 360));
    }

    // FastLED trig sampling helpers (explicit angle/phase conversions).
    constexpr uint16_t angleToFastLedPhase(BoundedAngle a) { return raw(a); }

    constexpr uint16_t angleToFastLedPhase(UnboundedAngle p) {
        return static_cast<uint16_t>(raw(p) >> 16);
    }
}

#endif // POLAR_SHADER_PIPELINE_MATHS_ANGLEMATHS_H
