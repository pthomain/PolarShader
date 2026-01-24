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

#include "renderer/pipeline/utils/StrongTypes.h"
#include "renderer/pipeline/units/UnitConstants.h"

namespace PolarShader {
    // --- Strong type tags ---
    struct AngleUnitsQ0_16_Tag {
    };

    struct AngleTurnsUQ16_16_Tag {
    };

    struct TrigQ1_15_Tag {
    };

    /**
    *   A phase angle expressed as a fraction of a full turn, quantized to 16 bits.
    */
    using BoundedAngle = Strong<uint16_t, AngleUnitsQ0_16_Tag>;

    /**
    *   A higher-resolution phase accumulator in AngleUnitsQ0_16 using unsigned Q16.16 semantics.
    */
    using UnboundedAngle = Strong<uint32_t, AngleTurnsUQ16_16_Tag>;

    inline constexpr UnboundedAngle ANGLE_TURNS_ONE_TURN = UnboundedAngle(Q16_16_ONE);

    /**
    *   The output of sin16/cos16 in signed fixed-point Q1.15.
    */
    using TrigQ1_15 = Strong<int16_t, TrigQ1_15_Tag>;

    // Convenience aliases for existing call sites/tests.
    using AngleUnitsQ0_16 = BoundedAngle;
    using AngleTurnsUQ16_16 = UnboundedAngle;

    // --- Raw extractors ---
    constexpr uint16_t raw(BoundedAngle a) { return a.raw(); }
    constexpr uint32_t raw(UnboundedAngle p) { return p.raw(); }
    constexpr int16_t raw(TrigQ1_15 t) { return t.raw(); }

    // --- Angle/phase promotion and sampling ---
    constexpr UnboundedAngle angleToPhase(BoundedAngle units) {
        return UnboundedAngle(static_cast<uint32_t>(raw(units)) << 16);
    }

    constexpr BoundedAngle phaseToAngle(UnboundedAngle turns) {
        return BoundedAngle(static_cast<uint16_t>(raw(turns) >> 16));
    }

    // --- Phase wrap arithmetic (mod 2^32) ---
    constexpr UnboundedAngle phaseWrapAdd(UnboundedAngle a, uint32_t delta) {
        return UnboundedAngle(raw(a) + delta);
    }

    // Wrap-add using signed raw delta (Q16.16), interpreted via two's-complement.
    constexpr UnboundedAngle phaseWrapAddSigned(UnboundedAngle a, int32_t delta_raw_q16_16) {
        return UnboundedAngle(raw(a) + static_cast<uint32_t>(delta_raw_q16_16));
    }

    // --- Phase quantise/fold helpers ---
    inline UnboundedAngle phaseMultiplyWrap(UnboundedAngle p, uint8_t k) {
        return UnboundedAngle(raw(p) * static_cast<uint32_t>(k));
    }

    inline UnboundedAngle phaseQuantise(UnboundedAngle p, uint32_t binWidth) {
        if (binWidth == 0) return p;
        uint32_t v = raw(p);
        uint32_t q = static_cast<uint32_t>((static_cast<uint64_t>(v) / binWidth) * binWidth);
        return UnboundedAngle(q);
    }
}

#endif // POLAR_SHADER_PIPELINE_UNITS_ANGLEUNITS_H
