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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_SCALARSIGNALS_H
#define POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_SCALARSIGNALS_H

#include <utility>
#include "FastLED.h"
#include "renderer/pipeline/maths/AngleMaths.h"
#include "renderer/pipeline/maths/NoiseMaths.h"
#include "PhaseAccumulator.h"

namespace PolarShader {
    /**
     * @brief Time-indexed scalar signal used for animation and modulation (unbounded).
     *
     * Use when:
     * - You need a value that changes over time (e.g., speed, amplitude, offsets).
     * - The consumer expects Q16.16 fixed-point semantics.
     *
     * Avoid when:
     * - You need angular phase; use UnboundedAngleSignal instead.
     * - You need per-frame stateful integration; use a modulator class instead.
     */
    using UnboundedScalarSignal = fl::function<UnboundedScalar(TimeMillis)>;

    /**
     * @brief Time-indexed scalar signal bounded to Q0.16.
     *
     * Use when:
     * - The consumer maps the output into its own min/max range.
     */
    using BoundedScalarSignal = fl::function<BoundedScalar(TimeMillis)>;

    /**
     * @brief Constant scalar signal (Q16.16).
     */
    inline UnboundedScalarSignal constant(UnboundedScalar value) {
        return [value](TimeMillis) { return value; };
    }

    /**
     * @brief Constant bounded scalar signal (Q0.16).
     */
    inline BoundedScalarSignal constant(BoundedScalar value) {
        return [value](TimeMillis) { return value; };
    }

    inline BoundedScalarSignal noise(
        BoundedScalarSignal phaseVelocity,
        BoundedScalarSignal amplitude,
        BoundedScalarSignal offset = constant(BoundedScalar(U16_HALF))
    ) {
        return createSignal(
            [phaseVelocity = std::move(phaseVelocity)](TimeMillis time) {
                return UnboundedScalar::fromRaw(static_cast<int32_t>(raw(phaseVelocity(time))));
            },
            std::move(amplitude),
            std::move(offset),
            [](UnboundedAngle phase) -> TrigQ1_15 {
                NoiseRawU16 rawNoise = NoiseRawU16(inoise16(angleToFastLedPhase(phase)));
                NoiseNormU16 normNoise = noiseNormaliseU16(rawNoise);
                int16_t signedNoise = static_cast<int16_t>(static_cast<int32_t>(raw(normNoise)) - U16_HALF);
                return TrigQ1_15(signedNoise);
            }
        );
    }
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_SCALARSIGNALS_H
