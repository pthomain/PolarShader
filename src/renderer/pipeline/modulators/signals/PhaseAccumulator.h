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

#include "renderer/pipeline/modulators/BoundUtils.h"
#include "renderer/pipeline/utils/MathUtils.h"
#include "renderer/pipeline/utils/TimeUtils.h"
#include "renderer/pipeline/utils/Units.h"

namespace PolarShader {

    // PhaseAccumulator assumes modulo-2^32 wrap semantics and is only valid for angular/phase domains.
    template<typename VelocitySignal>
    class PhaseAccumulator {
    public:
        UnboundedAngle phase{0};
        TimeMillis lastTime{0};
        bool hasLastTime{false};
        // phaseVelocity returns signed turns-per-second in Q16.16.
        VelocitySignal phaseVelocity;

        explicit PhaseAccumulator(
            VelocitySignal velocity,
            const UnboundedAngle initialPhase = UnboundedAngle(0)
        ) : phase(initialPhase),
            phaseVelocity(std::move(velocity)) {
        }

        UnboundedAngle advance(TimeMillis time) {
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

            UnboundedScalar dt_q16 = millisToUnboundedScalar(deltaTime);
            RawQ16_16 phase_advance = RawQ16_16(mul_q16_16_wrap(phaseVelocity(time), dt_q16).asRaw());
            phase = wrapAddSigned(phase, raw(phase_advance));
            return phase;
        }
    };

    template<typename PhaseVelocitySignal, typename AmplitudeSignal, typename OffsetSignal, typename SampleFn>
    fl::function<BoundedScalar(TimeMillis)> createSignal(
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
                ](TimeMillis time) mutable -> BoundedScalar {
            UnboundedAngle phase = acc.advance(time);
            TrigQ1_15 v = sample(phase);

            uint16_t sample_u16 = static_cast<uint16_t>(static_cast<int32_t>(raw(v)) + 0x8000);
            uint32_t amp = raw(amplitude(time));
            uint32_t off = raw(offset(time));

            uint32_t scaled = (static_cast<uint32_t>(sample_u16) * amp) >> 16;
            uint32_t sum = scaled + off;
            if (sum > 0xFFFFu) sum = 0xFFFFu;
            return BoundedScalar(static_cast<uint16_t>(sum));
        };
    }
} // namespace PolarShader

#endif // POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_PHASEACCUMULATOR_H
