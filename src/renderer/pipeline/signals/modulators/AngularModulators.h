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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_ANGULAR_H
#define POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_ANGULAR_H

#include "ScalarModulators.h"

namespace PolarShader {

    /**
     * @brief Time-indexed angular phase signal (turns in Q16.16).
     *
     * Use when:
     * - You need a canonical phase that preserves fractional turns.
     * - You will sample trig using AngleTurnsUQ16_16 overloads.
     *
     * Avoid when:
     * - You only need a raw AngleUnitsQ0_16 angle; promote at the boundary instead.
     */
    using AngleModulator = fl::function<AngleTurnsUQ16_16(TimeMillis)>;

    /**
     * @brief Constant angular phase.
     *
     * Use when:
     * - Direction is fixed.
     *
     * Avoid when:
     * - You need rotation over time; use IntegrateAngle.
     */
    inline AngleModulator ConstantAngleTurns(AngleTurnsUQ16_16 phase) {
        return [phase](TimeMillis) { return phase; };
    }

    /**
     * @brief Constant angular phase from AngleUnitsQ0_16.
     *
     * Use when:
     * - You have legacy angle units and need a constant direction.
     *
     * Avoid when:
     * - You want to return AngleUnitsQ0_16 from a modulator; keep phase at Q16.16.
     */
    inline AngleModulator ConstantAngleUnits(AngleUnitsQ0_16 angle) {
        return ConstantAngleTurns(angleUnitsToAngleTurns(angle));
    }

    /**
     * @brief Integrate angular velocity (turns/sec) into phase.
     *
     * Use when:
     * - You have a ScalarModulator representing angular velocity.
     * - You need smooth phase accumulation with wrap semantics.
     *
     * Avoid when:
     * - You already have phase; use ConstantAngleTurns or a custom modulator.
     */
    inline AngleModulator IntegrateAngle(
        ScalarModulator angularVelocityTurnsPerSec,
        AngleTurnsUQ16_16 initialPhase = AngleTurnsUQ16_16(0)
    ) {
        detail::PhaseAccumulator acc{std::move(angularVelocityTurnsPerSec)};
        acc.phase = initialPhase;
        return [acc = std::move(acc)](TimeMillis time) mutable -> AngleTurnsUQ16_16 {
            return acc.advance(time);
        };
    }
}

#endif //POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_ANGULAR_H
