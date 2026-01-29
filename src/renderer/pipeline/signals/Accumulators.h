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

#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/units/CartesianUnits.h"

namespace PolarShader {
    /**
     * @brief Time-indexed depth signal in unsigned Q24.8 domain.
     *
     * Typically used for animated noise depth/phase offsets.
     */
    using DepthSignal = fl::function<uint32_t(TimeMillis)>;

    // PhaseAccumulator wraps in 16-bit turn space and is only valid for angular/phase domains.
    class PhaseAccumulator {
    public:
        explicit PhaseAccumulator(
            MappedSignal<SFracQ0_16> speed,
            SFracQ0_16 initialPhase = SFracQ0_16(0)
        );

        SFracQ0_16 advance(TimeMillis time);

    private:
        uint32_t phaseRaw32{0};
        TimeMillis lastTime{0};
        bool hasLastTime{false};
        // phaseSpeed returns turns-per-second in Q0.16.
        MappedSignal<SFracQ0_16> phaseSpeed;
    };

    class CartesianMotionAccumulator {
    public:
        /**
         * @brief Integrates direction + speed into a cartesian position.
         *
         * Direction is a mapped angle in turns; speed is mapped to units/sec.
         */
        CartesianMotionAccumulator(
            SPoint32 initialPosition,
            MappedSignal<FracQ0_16> direction,
            MappedSignal<int32_t> speed
        );

        SPoint32 advance(TimeMillis time);

        SPoint32 position() const { return pos; }

    private:
        SPoint32 pos{0, 0};
        TimeMillis lastTime{0};
        bool hasLastTime{false};
        MappedSignal<FracQ0_16> directionSignal;
        MappedSignal<int32_t> speedSignal;
    };
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_ACCUMULATORS_H
