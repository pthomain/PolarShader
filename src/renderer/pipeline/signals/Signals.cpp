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
#include "renderer/pipeline/maths/ScalarMaths.h"
#include <cstdint>
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
            int32_t amp_raw = raw(amplitude(time));
            int32_t off_raw = raw(offset(time));
            uint32_t amp = (amp_raw < 0) ? 0u : static_cast<uint32_t>(amp_raw);
            if (amp > FRACT_Q0_16_MAX) amp = FRACT_Q0_16_MAX;

            uint32_t sample_u16 = static_cast<uint32_t>((sample_raw + Q0_16_ONE) >> 1);
            uint32_t off = (off_raw < 0) ? 0u : static_cast<uint32_t>(off_raw);
            if (off > FRACT_Q0_16_MAX) off = FRACT_Q0_16_MAX;
            uint32_t scaled = (sample_u16 * amp) >> 16;
            uint32_t sum = scaled + off;
            if (sum > 0xFFFFu) sum = 0xFFFFu;
            return SFracQ0_16(static_cast<int32_t>(sum));
        };
    }

    SFracQ0_16Signal floor() {
        return constant(frac(0));
    }

    SFracQ0_16Signal midPoint() {
        return constant(frac(2));
    }

    SFracQ0_16Signal ceiling() {
        return constant(frac(1));
    }

    SFracQ0_16Signal constant(SFracQ0_16 value) {
        return [value](TimeMillis) { return value; };
    }

    SFracQ0_16Signal constant(FracQ0_16 value) {
        return [value](TimeMillis) { return SFracQ0_16(raw(value)); };
    }

    SFracQ0_16Signal cFrac(uint32_t value) {
        return constant(frac(value));
    }

    SFracQ0_16Signal cSFrac(uint32_t value) {
        return constant(sFrac(value));
    }

    SFracQ0_16Signal cPerMil(uint16_t value) {
        return constant(perMil(value));
    }

    SFracQ0_16Signal full() {
        return constant(frac(1));
    }

    SFracQ0_16Signal noise(
        const SFracQ0_16Signal &phaseVelocity,
        SFracQ0_16Signal amplitude,
        SFracQ0_16Signal offset
    ) {
        return createSignal(
            phaseVelocity,
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

    namespace {
        uint32_t normalizeLooped(TimeMillis time, TimeMillis durationMs) {
            if (durationMs == 0) return FRACT_Q0_16_MAX;
            TimeMillis t = time % durationMs;
            uint64_t scaled = (static_cast<uint64_t>(t) * FRACT_Q0_16_MAX) / durationMs;
            if (scaled > FRACT_Q0_16_MAX) scaled = FRACT_Q0_16_MAX;
            return static_cast<uint32_t>(scaled);
        }

        uint32_t easeInQuadRaw(uint32_t t) {
            uint64_t tt = static_cast<uint64_t>(t) * static_cast<uint64_t>(t);
            return static_cast<uint32_t>(tt >> 16);
        }

        uint32_t easeOutQuadRaw(uint32_t t) {
            uint32_t one = Q0_16_ONE;
            uint32_t two = Q0_16_ONE * 2u;
            uint64_t value = static_cast<uint64_t>(t) * static_cast<uint64_t>(two - t);
            uint32_t result = static_cast<uint32_t>(value >> 16);
            return (result > one) ? one : result;
        }

        uint32_t easeInOutQuadRaw(uint32_t t) {
            if (t < U16_HALF) {
                uint64_t tt = static_cast<uint64_t>(t) * static_cast<uint64_t>(t);
                return static_cast<uint32_t>(tt >> 15);
            }
            uint32_t one = Q0_16_ONE;
            uint32_t u = one - t;
            uint64_t uu = static_cast<uint64_t>(u) * static_cast<uint64_t>(u);
            uint32_t tail = static_cast<uint32_t>(uu >> 15);
            return (tail > one) ? 0u : (one - tail);
        }
    } // namespace

    SFracQ0_16Signal linear(TimeMillis durationMs) {
        return [durationMs](TimeMillis time) {
            return SFracQ0_16(static_cast<int32_t>(normalizeLooped(time, durationMs)));
        };
    }

    SFracQ0_16Signal easeIn(TimeMillis durationMs) {
        return [durationMs](TimeMillis time) {
            return SFracQ0_16(static_cast<int32_t>(easeInQuadRaw(normalizeLooped(time, durationMs))));
        };
    }

    SFracQ0_16Signal easeOut(TimeMillis durationMs) {
        return [durationMs](TimeMillis time) {
            return SFracQ0_16(static_cast<int32_t>(easeOutQuadRaw(normalizeLooped(time, durationMs))));
        };
    }

    SFracQ0_16Signal easeInOut(TimeMillis durationMs) {
        return [durationMs](TimeMillis time) {
            return SFracQ0_16(static_cast<int32_t>(easeInOutQuadRaw(normalizeLooped(time, durationMs))));
        };
    }

    SFracQ0_16Signal invert(SFracQ0_16Signal signal) {
        return [signal = std::move(signal)](TimeMillis time) mutable {
            int32_t value = raw(signal(time));
            const int32_t max = static_cast<int32_t>(FRACT_Q0_16_MAX);
            if (value <= 0) return SFracQ0_16(max);
            if (value >= max) return SFracQ0_16(0);
            return SFracQ0_16(max - value);
        };
    }

    SFracQ0_16Signal scale(SFracQ0_16Signal signal, FracQ0_16 factor) {
        return [signal = std::move(signal), factor](TimeMillis time) mutable {
            return mulSFracSat(signal(time), SFracQ0_16(raw(factor)));
        };
    }

    DepthSignal constantDepth(uint32_t value) {
        return [value](TimeMillis) { return value; };
    }

    DepthSignal depth(
        SFracQ0_16Signal signal,
        DepthRange range
    ) {
        return [signal = std::move(signal), range = std::move(range)](TimeMillis time) mutable -> uint32_t {
            return range.map(signal(time)).get();
        };
    }

    DepthSignal depth(
        SFracQ0_16Signal signal,
        uint32_t scale,
        uint32_t offset
    ) {
        uint64_t max = static_cast<uint64_t>(offset) + static_cast<uint64_t>(scale);
        uint32_t max_depth = (max > UINT32_MAX) ? UINT32_MAX : static_cast<uint32_t>(max);
        return depth(std::move(signal), DepthRange(offset, max_depth));
    }
}
