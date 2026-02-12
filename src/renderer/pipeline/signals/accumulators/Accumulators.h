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

namespace PolarShader {
    /**
     * @brief Progress-indexed depth signal in unsigned r8 domain.
     *
     * Typically used for animated noise depth/phase offsets.
     */
    using DepthSignal = fl::function<uint32_t(f16, TimeMillis)>;

    // PhaseAccumulator wraps in 16-bit turn space and is only valid for angular/phase domains.
    class PhaseAccumulator {
    public:
        using SpeedSampleFn = Sf16Signal::WaveformFn;

        explicit PhaseAccumulator(
            SpeedSampleFn speed,
            f16 initialPhase = f16(0)
        );

        f16 advance(TimeMillis elapsedMs);

    private:
        uint32_t phaseRaw32{0};
        TimeMillis lastElapsedMs{0};
        bool hasLastElapsed{false};
        // phaseSpeed returns turns-per-second
        SpeedSampleFn phaseSpeed;
    };
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_ACCUMULATORS_H
