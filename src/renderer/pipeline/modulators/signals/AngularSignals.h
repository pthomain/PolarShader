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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_ANGULARSIGNALS_H
#define POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_ANGULARSIGNALS_H

#include <utility>
#include "ScalarSignals.h"
#include "renderer/pipeline/maths/AngleMaths.h"

namespace PolarShader {
    /**
     * @brief Time-indexed angular phase signal (turns in Q16.16).
     *
     * Use when:
     * - You need a canonical phase that preserves fractional turns.
     * - You will sample trig using UnboundedAngle overloads.
     *
     * Avoid when:
     * - You only need a raw BoundedAngle; promote at the boundary instead.
     */
    using UnboundedAngleSignal = fl::function<UnboundedAngle(TimeMillis)>;

    /**
     * @brief Time-indexed angular signal bounded to BoundedAngle.
     */
    using BoundedAngleSignal = fl::function<BoundedAngle(TimeMillis)>;

    /**
     * @brief Constant angular phase.
     */
    inline UnboundedAngleSignal constant(UnboundedAngle phase) {
        return [phase](TimeMillis) { return phase; };
    }

    /**
     * @brief Constant angular phase from BoundedAngle.
     */
    inline BoundedAngleSignal constant(BoundedAngle angle) {
        return [angle](TimeMillis) { return angle; };
    }

    inline BoundedAngleSignal sine(
        BoundedScalarSignal phaseVelocity,
        BoundedScalarSignal amplitude,
        BoundedScalarSignal offset = constant(BoundedScalar(U16_HALF))
    ) {
        auto signal = createSignal( //TODO use existing mapper
            [phaseVelocity = std::move(phaseVelocity)](TimeMillis time) {
                return UnboundedScalar::fromRaw(static_cast<int32_t>(raw(phaseVelocity(time))));
            },
            std::move(amplitude),
            std::move(offset),
            [](UnboundedAngle phase) -> TrigQ1_15 {
                return angleSinQ1_15(phase);
            }
        );
        return [signal = std::move(signal)](TimeMillis time) {
            return BoundedAngle(raw(signal(time)));
        };
    }

    inline BoundedAngleSignal pulse(
        BoundedScalarSignal phaseVelocity,
        BoundedScalarSignal amplitude,
        BoundedScalarSignal offset = constant(BoundedScalar(U16_HALF))
    ) {
        auto signal = createSignal(//TODO use existing mapper
            [phaseVelocity = std::move(phaseVelocity)](TimeMillis time) {
                return UnboundedScalar::fromRaw(static_cast<int32_t>(raw(phaseVelocity(time))));
            },
            std::move(amplitude),
            std::move(offset),
            [](UnboundedAngle phase) -> TrigQ1_15 {
                BoundedAngle saw = phaseToAngle(phase);
                uint16_t saw_raw = raw(saw);
                uint16_t pulse_raw = (saw_raw < HALF_TURN_U16)
                                         ? static_cast<uint16_t>(saw_raw << 1)
                                         : static_cast<uint16_t>((ANGLE_U16_MAX - saw_raw) << 1);
                int16_t signedPulse = static_cast<int16_t>(static_cast<int32_t>(pulse_raw) - U16_HALF);
                return TrigQ1_15(signedPulse);
            }
        );
        return [signal = std::move(signal)](TimeMillis time) {
            return BoundedAngle(raw(signal(time)));
        };
    }
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_ANGULARSIGNALS_H
