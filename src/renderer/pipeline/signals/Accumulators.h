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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_ACCUMULATORS_H
#define POLAR_SHADER_PIPELINE_SIGNALS_ACCUMULATORS_H

#include "FastLED.h"
#include "renderer/pipeline/units/ScalarUnits.h"
#include "renderer/pipeline/units/TimeUnits.h"
#include "renderer/pipeline/units/Range.h"

namespace PolarShader {
    /**
     * @brief Time-indexed scalar signal bounded to Q0.16.
     *
     * Use when:
     * - The consumer maps the output into its own min/max range.
     */
    using SFracQ0_16Signal = fl::function<SFracQ0_16(TimeMillis)>;

    // PhaseAccumulator wraps in 16-bit turn space and is only valid for angular/phase domains.
    class PhaseAccumulator {
    public:
        explicit PhaseAccumulator(
            SFracQ0_16Signal velocity,
            SFracQ0_16 initialPhase = SFracQ0_16(0)
        );

        SFracQ0_16 advance(TimeMillis time);

    private:
        SFracQ0_16 phase{0};
        TimeMillis lastTime{0};
        bool hasLastTime{false};
        // phaseVelocity returns turns-per-second in Q0.16.
        SFracQ0_16Signal phaseVelocity;
    };

    class CartesianMotionAccumulator {
    public:
        /**
         * @brief Integrates direction + velocity into a cartesian position.
         *
         * velocityRange radius is the max speed in units/sec.
         */
        CartesianMotionAccumulator(
            SPoint32 initialPosition,
            Range velocityRange,
            SFracQ0_16Signal direction,
            SFracQ0_16Signal velocity
        );

        SPoint32 advance(TimeMillis time);

        SPoint32 position() const { return pos; }

    private:
        SPoint32 pos{0, 0};
        TimeMillis lastTime{0};
        bool hasLastTime{false};
        Range velocityRange;
        SFracQ0_16Signal directionSignal;
        SFracQ0_16Signal velocitySignal;
    };
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_ACCUMULATORS_H
