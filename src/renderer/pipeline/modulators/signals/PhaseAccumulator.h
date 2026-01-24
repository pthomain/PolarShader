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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_PHASEACCUMULATOR_H
#define POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_PHASEACCUMULATOR_H

#include <utility>

#include "renderer/pipeline/maths/Maths.h"
#include "renderer/pipeline/units/Units.h"

namespace PolarShader {

    // PhaseAccumulator wraps in 16-bit turn space and is only valid for angular/phase domains.
    template<typename VelocitySignal>
    class PhaseAccumulator {
    public:
        AngleQ0_16 phase{0};
        TimeMillis lastTime{0};
        bool hasLastTime{false};
        // phaseVelocity returns signed turns-per-second in Q0.16.
        VelocitySignal phaseVelocity;

        explicit PhaseAccumulator(
            VelocitySignal velocity,
            const AngleQ0_16 initialPhase = AngleQ0_16(0)
        ) : phase(initialPhase),
            phaseVelocity(std::move(velocity)) {
        }

        AngleQ0_16 advance(TimeMillis time) {
            if (!hasLastTime) {
                lastTime = time;
                hasLastTime = true;
                return phase;
            }
            TimeMillis deltaTime = time - lastTime;
            lastTime = time;
            if (deltaTime == 0) return phase;

            deltaTime = clampDeltaTime(deltaTime);
            if (deltaTime == 0) return phase;

            ScalarQ0_16 dt_q0_16 = timeMillisToScalar(deltaTime);
            ScalarQ0_16 phase_advance = scalarMulQ0_16Wrap(phaseVelocity(time), dt_q0_16);
            phase = angleWrapAddSigned(phase, raw(phase_advance));
            return phase;
        }
    };

    template<typename PhaseVelocitySignal, typename AmplitudeSignal, typename OffsetSignal, typename SampleFn>
    fl::function<FracQ0_16(TimeMillis)> createSignal(
        PhaseVelocitySignal phaseVelocity,
        AmplitudeSignal amplitude,
        OffsetSignal offset,
        SampleFn sample
    ) {
        PhaseAccumulator acc{std::move(phaseVelocity)};

        return [acc = std::move(acc),
                    amplitude = std::move(amplitude),
                    offset = std::move(offset),
                    sample = std::move(sample)
                ](TimeMillis time) mutable -> FracQ0_16 {
            AngleQ0_16 phase = acc.advance(time);
            TrigQ0_16 v = sample(phase);

            int32_t sample_raw = raw(v);
            uint32_t sample_u16 = static_cast<uint32_t>((sample_raw + Q0_16_ONE) >> 1);
            uint32_t amp = raw(amplitude(time));
            uint32_t off = raw(offset(time));

            uint32_t scaled = (sample_u16 * amp) >> 16;
            uint32_t sum = scaled + off;
            if (sum > 0xFFFFu) sum = 0xFFFFu;
            return FracQ0_16(static_cast<uint16_t>(sum));
        };
    }
} // namespace PolarShader

#endif // POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_PHASEACCUMULATOR_H
