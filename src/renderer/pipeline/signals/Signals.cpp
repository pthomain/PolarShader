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
#include "renderer/pipeline/ranges/LinearRange.h"
#include "renderer/pipeline/ranges/UVRange.h"
#include <cstdint>
#include <utility>
#include <memory>

namespace PolarShader {
    namespace {
        /**
         * @brief Creates a signal that evolves based on a speed signal.
         * 
         * Uses a PhaseAccumulator to integrate speed over time, supporting dynamic 
         * and bidirectional motion. Independent of scene duration.
         */
        SFracQ0_16Signal createSignal(
            SFracQ0_16Signal speed,
            SFracQ0_16Signal amplitude,
            SFracQ0_16Signal offset,
            SampleSignal sample
        ) {
            // Convert SFracQ0_16Signal to MappedSignal<SFracQ0_16> for the accumulator.
            auto speedMapped = MappedSignal<SFracQ0_16>(
                [speed](FracQ0_16 p, TimeMillis t) {
                    return MappedValue(speed(p, t));
                },
                speed.isAbsolute()
            );

            PhaseAccumulator acc{
                std::move(speedMapped)
            };

            return SFracQ0_16Signal(
                [acc = std::move(acc),
                    amplitude = std::move(amplitude),
                    offset = std::move(offset),
                    sample = std::move(sample)
                ](FracQ0_16 progress, TimeMillis elapsedMs) mutable -> SFracQ0_16 {
                    FracQ0_16 phase = acc.advance(progress, elapsedMs);
                    SFracQ0_16 v = sample(phase);

                    int32_t sample_raw = raw(v);
                    int32_t amp_raw = raw(amplitude(progress, elapsedMs));
                    int32_t off_raw = raw(offset(progress, elapsedMs));

                    // Sample is already [0, 65535] (representing 0..1)
                    uint32_t sample_u16 = static_cast<uint32_t>(sample_raw);
                    if (sample_raw < 0) sample_u16 = 0;
                    if (sample_raw > 0xFFFF) sample_u16 = 0xFFFF;

                    uint32_t amp = (amp_raw < 0) ? 0u : static_cast<uint32_t>(amp_raw);
                    if (amp > FRACT_Q0_16_MAX) amp = FRACT_Q0_16_MAX;

                    uint32_t off = (off_raw < 0) ? 0u : static_cast<uint32_t>(off_raw);
                    if (off > FRACT_Q0_16_MAX) off = FRACT_Q0_16_MAX;

                    uint32_t scaled = (amp >= FRACT_Q0_16_MAX)
                        ? sample_u16
                        : (sample_u16 * amp) >> 16;
                    uint32_t sum = scaled + off;
                    if (sum > 0xFFFFu) sum = 0xFFFFu;
                    return SFracQ0_16(static_cast<int32_t>(sum));
                });
        }

        /**
         * @brief Calculates progress (0..1) for a signal.
         * 
         * @param sceneProgress The global progress of the current scene (0..1).
         * @param elapsedMs The time elapsed since the start of the current scene.
         * @param period The signal's specific duration. If 0, uses sceneProgress.
         */
        FracQ0_16 calculateProgress(FracQ0_16 sceneProgress, TimeMillis elapsedMs, Period period) {
            if (period == 0) return sceneProgress;
            uint32_t p = (static_cast<uint64_t>(elapsedMs % period) * 0xFFFFu) / period;
            return FracQ0_16(static_cast<uint16_t>(p));
        }
    }

    SFracQ0_16Signal floor() {
        return constant(frac(0));
    }

    SFracQ0_16Signal midPoint() {
        return constant(frac(1, 2));
    }

    SFracQ0_16Signal ceiling() {
        return constant(frac(1));
    }

    SFracQ0_16Signal constant(SFracQ0_16 value) {
        return SFracQ0_16Signal([value](FracQ0_16, TimeMillis) { return value; });
    }

    SFracQ0_16Signal constant(FracQ0_16 value) {
        return SFracQ0_16Signal([value](FracQ0_16, TimeMillis) { return SFracQ0_16(raw(value)); });
    }

    SFracQ0_16Signal cFrac(int32_t value) {
        int64_t val = (static_cast<int64_t>(value) * 65536LL) / 100LL;
        return constant(SFracQ0_16(static_cast<int32_t>(val)));
    }

    SFracQ0_16Signal cPerMil(int32_t value) {
        int64_t val = (static_cast<int64_t>(value) * 65536LL) / 1000LL;
        return constant(SFracQ0_16(static_cast<int32_t>(val)));
    }

    SFracQ0_16Signal randomPerMil() {
        return cPerMil(1000 * random16() / UINT16_MAX);
    }

    SFracQ0_16Signal linear(Period period) {
        return SFracQ0_16Signal([period](FracQ0_16 p, TimeMillis t) { 
            return SFracQ0_16(raw(calculateProgress(p, t, period))); 
        });
    }

    SFracQ0_16Signal quadraticIn(Period period) {
        return SFracQ0_16Signal([period](FracQ0_16 progress, TimeMillis t) {
            uint32_t p = raw(calculateProgress(progress, t, period));
            uint32_t result = (p * p) >> 16;
            return SFracQ0_16(static_cast<int32_t>(result));
        });
    }

    SFracQ0_16Signal quadraticOut(Period period) {
        return SFracQ0_16Signal([period](FracQ0_16 progress, TimeMillis t) {
            uint32_t p = raw(calculateProgress(progress, t, period));
            uint32_t inv = 0xFFFFu - p;
            uint32_t result = 0xFFFFu - ((inv * inv) >> 16);
            return SFracQ0_16(static_cast<int32_t>(result));
        });
    }

    SFracQ0_16Signal quadraticInOut(Period period) {
        return SFracQ0_16Signal([period](FracQ0_16 progress, TimeMillis t) {
            uint32_t p = raw(calculateProgress(progress, t, period));
            if (p < 0x8000u) {
                uint32_t result = (p * p) >> 15;
                return SFracQ0_16(static_cast<int32_t>(result));
            } else {
                uint32_t inv = 0xFFFFu - p;
                uint32_t tail = (inv * inv) >> 15;
                return SFracQ0_16(static_cast<int32_t>(0xFFFFu - tail));
            }
        });
    }

    SFracQ0_16Signal sinusoidalIn(Period period) {
        return SFracQ0_16Signal([period](FracQ0_16 progress, TimeMillis t) {
            uint16_t theta = raw(calculateProgress(progress, t, period)) >> 2;
            int16_t c = cos16(theta);
            uint32_t result = 0x7FFFf - c;
            return SFracQ0_16(static_cast<int32_t>(result << 1));
        });
    }

    SFracQ0_16Signal sinusoidalOut(Period period) {
        return SFracQ0_16Signal([period](FracQ0_16 progress, TimeMillis t) {
            uint16_t theta = raw(calculateProgress(progress, t, period)) >> 2;
            int16_t s = sin16(theta);
            return SFracQ0_16(static_cast<int32_t>(static_cast<uint32_t>(s) << 1));
        });
    }

    SFracQ0_16Signal sinusoidalInOut(Period period) {
        return SFracQ0_16Signal([period](FracQ0_16 progress, TimeMillis t) {
            uint16_t theta = raw(calculateProgress(progress, t, period)) >> 1;
            int16_t c = cos16(theta);
            int32_t result = 0x7FFF - c;
            return SFracQ0_16(result);
        });
    }

    SFracQ0_16Signal noise(
        SFracQ0_16Signal speed,
        SFracQ0_16Signal amplitude,
        SFracQ0_16Signal offset
    ) {
        return createSignal(
            std::move(speed),
            std::move(amplitude),
            std::move(offset),
            sampleNoise()
        );
    }

    SFracQ0_16Signal sine(
        SFracQ0_16Signal speed,
        SFracQ0_16Signal amplitude,
        SFracQ0_16Signal offset
    ) {
        return createSignal(
            std::move(speed),
            std::move(amplitude),
            std::move(offset),
            sampleSine()
        );
    }

    SFracQ0_16Signal invert(SFracQ0_16Signal signal) {
        return SFracQ0_16Signal([signal = std::move(signal)](FracQ0_16 progress, TimeMillis elapsedMs) mutable {
            int32_t value = raw(signal(progress, elapsedMs));
            const int32_t max = FRACT_Q0_16_MAX;
            if (value <= 0) return SFracQ0_16(max);
            if (value >= max) return SFracQ0_16(0);
            return SFracQ0_16(max - value);
        });
    }

    SFracQ0_16Signal scale(SFracQ0_16Signal signal, FracQ0_16 factor) {
        return SFracQ0_16Signal([signal = std::move(signal), factor](FracQ0_16 progress, TimeMillis elapsedMs) mutable {
            return mulSFracSat(signal(progress, elapsedMs), SFracQ0_16(raw(factor)));
        });
    }

    UVSignal constantUV(UV value) {
        return UVSignal([value](FracQ0_16, TimeMillis) { return value; });
    }

    UVSignal uvSignal(SFracQ0_16Signal u, SFracQ0_16Signal v) {
        bool absolute = u.isAbsolute() && v.isAbsolute();
        return UVSignal([u = std::move(u), v = std::move(v)](FracQ0_16 progress, TimeMillis elapsedMs) mutable {
            return UV(
                FracQ16_16(raw(u(progress, elapsedMs))),
                FracQ16_16(raw(v(progress, elapsedMs)))
            );
        }, absolute);
    }

    UVSignal uv(SFracQ0_16Signal signal, UV min, UV max) {
        UVRange range(min, max);
        auto mapped = range.mapSignal(std::move(signal));
        return UVSignal([mapped = std::move(mapped)](FracQ0_16 progress, TimeMillis elapsedMs) mutable {
            return mapped(progress, elapsedMs).get();
        }, mapped.isAbsolute());
    }

    DepthSignal constantDepth(uint32_t value) {
        return [value](FracQ0_16, TimeMillis) { return value; };
    }

    DepthSignal depth(
        SFracQ0_16Signal signal,
        LinearRange<uint32_t> range
    ) {
        auto mapped = resolveMappedSignal(range.mapSignal(std::move(signal)));
        return [mapped = std::move(mapped)](FracQ0_16 progress, TimeMillis elapsedMs) mutable -> uint32_t {
            return mapped(progress, elapsedMs).get();
        };
    }

    DepthSignal depth(
        SFracQ0_16Signal signal,
        uint32_t scale,
        uint32_t offset
    ) {
        uint64_t max = static_cast<uint64_t>(offset) + static_cast<uint64_t>(scale);
        uint32_t max_depth = (max > UINT32_MAX) ? UINT32_MAX : static_cast<uint32_t>(max);
        return depth(std::move(signal), LinearRange(offset, max_depth));
    }
}