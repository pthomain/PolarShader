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
    // Applies a Q0.16 scale to an int32 raw value (units are caller-defined).
    fl::i32 scalarScaleByBounded(fl::i32 value, BoundedScalar scale);

    UnboundedScalar scalarMulQ16_16Sat(UnboundedScalar a, UnboundedScalar b);

    UnboundedScalar scalarMulQ16_16Wrap(UnboundedScalar a, UnboundedScalar b);

    RawQ16_16 scalarAddWrapQ16_16(RawQ16_16 a, RawQ16_16 b);

    uint16_t scalarSqrtU32(uint32_t value);

    uint64_t scalarSqrtU64(uint64_t value);

    int64_t scalarScaleQ16_16ByTrig(RawQ16_16 magnitude, TrigQ1_15 trig_q1_15);

    UnboundedScalar scalarClampQ16_16Raw(int64_t raw_value);

    /**
     * @brief Create a Q16.16 value from a rational fraction without floating point.
     */
    // constexpr UnboundedScalar scalarFromRatioUnbounded(int32_t num, uint32_t den) {
    //     if (den == 0) return UnboundedScalar(0);
    //     return UnboundedScalar::fromRaw(
    //         static_cast<int32_t>((static_cast<int64_t>(num) << 16) / static_cast<int64_t>(den))
    //     );
    // }

    /**
     * @brief Create a Q0.16 bounded scalar from a rational fraction without floating point.
     */
    constexpr BoundedScalar boundedFrac(uint32_t numerator, uint32_t denominator) {
        if (numerator == 0 || denominator == 0) return BoundedScalar(0);
        uint64_t raw_value = (static_cast<uint64_t>(FRACT_Q0_16_MAX) * numerator) / denominator;
        if (raw_value > FRACT_Q0_16_MAX) raw_value = FRACT_Q0_16_MAX;
        return BoundedScalar(static_cast<uint16_t>(raw_value));
    }

    constexpr BoundedScalar boundedFrac(uint32_t denominator) {
        return boundedFrac(1u, denominator);
    }

    constexpr BoundedScalar boundedPerMil(uint16_t perMil) {
        if (perMil == 0) return BoundedScalar(0);
        uint32_t raw_value = static_cast<uint32_t>((static_cast<uint64_t>(FRACT_Q0_16_MAX) * perMil) / 1000);
        return BoundedScalar(static_cast<uint16_t>(raw_value));
    }

    /**
     * @brief Map a bounded scalar into an unbounded scalar range.
     */
    UnboundedScalar unbound(BoundedScalar t, UnboundedScalar min_val, UnboundedScalar max_val);

    /**
     * @brief Map an unbounded scalar into a bounded scalar range.
     */
    BoundedScalar bound(UnboundedScalar value, UnboundedScalar min_val, UnboundedScalar max_val);
}

#endif // POLAR_SHADER_PIPELINE_MATHS_SCALARMATHS_H
