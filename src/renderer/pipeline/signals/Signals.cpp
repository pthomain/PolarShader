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

#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/signals/Accumulators.h"
#include "renderer/pipeline/signals/SignalSamplers.h"
#include <utility>

namespace PolarShader {
    SFracQ0_16Signal createSignal(
        SFracQ0_16Signal phaseVelocity,
        SFracQ0_16Signal amplitude,
        SFracQ0_16Signal offset,
        SampleSignal sample
    ) {
        PhaseAccumulator acc{std::move(phaseVelocity)};

        return [acc = std::move(acc),
                    amplitude = std::move(amplitude),
                    offset = std::move(offset),
                    sample = std::move(sample)
                ](TimeMillis time) mutable -> SFracQ0_16 {
            SFracQ0_16 phase = acc.advance(time);
            TrigQ0_16 v = sample(phase);

            int32_t sample_raw = raw(v);
            uint32_t sample_u16 = static_cast<uint32_t>((sample_raw + Q0_16_ONE) >> 1);
            int32_t amp_raw = raw(amplitude(time));
            int32_t off_raw = raw(offset(time));
            uint32_t amp = (amp_raw < 0) ? 0u : static_cast<uint32_t>(amp_raw);
            uint32_t off = (off_raw < 0) ? 0u : static_cast<uint32_t>(off_raw);
            if (amp > FRACT_Q0_16_MAX) amp = FRACT_Q0_16_MAX;
            if (off > FRACT_Q0_16_MAX) off = FRACT_Q0_16_MAX;

            uint32_t scaled = (sample_u16 * amp) >> 16;
            uint32_t sum = scaled + off;
            if (sum > 0xFFFFu) sum = 0xFFFFu;
            return SFracQ0_16(static_cast<int32_t>(sum));
        };
    }

    SFracQ0_16Signal constant(SFracQ0_16 value) {
        return [value](TimeMillis) { return value; };
    }

    SFracQ0_16Signal constant(FracQ0_16 value) {
        return [value](TimeMillis) { return SFracQ0_16(raw(value)); };
    }

    SFracQ0_16Signal noise(
        SFracQ0_16Signal phaseVelocity,
        SFracQ0_16Signal amplitude,
        SFracQ0_16Signal offset
    ) {
        return createSignal(
            std::move(phaseVelocity),
            std::move(amplitude),
            std::move(offset),
            sampleNoise()
        );
    }

    SFracQ0_16Signal sine(
        SFracQ0_16Signal phaseVelocity,
        SFracQ0_16Signal amplitude,
        SFracQ0_16Signal offset
    ) {
        return createSignal(
            std::move(phaseVelocity),
            std::move(amplitude),
            std::move(offset),
            sampleSine()
        );
    }

    SFracQ0_16Signal pulse(
        SFracQ0_16Signal phaseVelocity,
        SFracQ0_16Signal amplitude,
        SFracQ0_16Signal offset
    ) {
        return createSignal(
            std::move(phaseVelocity),
            std::move(amplitude),
            std::move(offset),
            samplePulse()
        );
    }
}
