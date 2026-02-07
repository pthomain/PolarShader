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
#include "renderer/pipeline/signals/SignalAccumulators.h"
#include "renderer/pipeline/signals/SignalSamplers.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include "renderer/pipeline/ranges/LinearRange.h"
#include "renderer/pipeline/ranges/UVRange.h"
#include <cstdint>
#include <limits>
#include <utility>

namespace PolarShader {
    namespace {
        constexpr int32_t SIGNAL_SPEED_SCALE = 1;

        SFracQ0_16 clamp01(SFracQ0_16 value) {
            return SFracQ0_16(static_cast<int32_t>(clamp_frac_raw(raw(value))));
        }

        FracQ0_16 timeToProgress(TimeMillis t, TimeMillis duration) {
            if (duration == 0) return FracQ0_16(0);
            if (t >= duration) t = duration;
            uint64_t scaled = (static_cast<uint64_t>(t) * FRACT_Q0_16_MAX) / duration;
            if (scaled > FRACT_Q0_16_MAX) scaled = FRACT_Q0_16_MAX;
            return FracQ0_16(static_cast<uint16_t>(scaled));
        }

        SFracQ0_16Signal createAperiodicSignal(
            TimeMillis duration,
            LoopMode loopMode,
            SFracQ0_16Signal::WaveformFn waveform
        ) {
            return SFracQ0_16Signal(SignalKind::APERIODIC, loopMode, duration, std::move(waveform));
        }

        SFracQ0_16Signal createPeriodicSignal(
            SFracQ0_16Signal speed,
            SFracQ0_16Signal amplitude,
            SFracQ0_16Signal offset,
            SFracQ0_16Signal phaseOffset,
            SampleSignal sample
        ) {
            auto speedMapped = MappedSignal<SFracQ0_16>(
                [speed = std::move(speed)](FracQ0_16, TimeMillis elapsedMs) mutable {
                    int64_t speedRaw = static_cast<int64_t>(raw(speed.sampleUnclamped(elapsedMs))) * SIGNAL_SPEED_SCALE;
                    if (speedRaw > std::numeric_limits<int32_t>::max()) speedRaw = std::numeric_limits<int32_t>::max();
                    if (speedRaw < std::numeric_limits<int32_t>::min()) speedRaw = std::numeric_limits<int32_t>::min();
                    return MappedValue(SFracQ0_16(static_cast<int32_t>(speedRaw)));
                }
            );

            PhaseAccumulator acc(std::move(speedMapped));

            return SFracQ0_16Signal(
                SignalKind::PERIODIC,
                [acc = std::move(acc),
                    amplitude = std::move(amplitude),
                    offset = std::move(offset),
                    phaseOffset = std::move(phaseOffset),
                    sample = std::move(sample)](TimeMillis elapsedMs) mutable -> SFracQ0_16 {
                    FracQ0_16 phase = acc.advance(FracQ0_16(0), elapsedMs);
                    uint32_t basePhase = raw(phase);
                    uint32_t phaseOffsetRaw = clamp_frac_raw(raw(phaseOffset(elapsedMs)));
                    FracQ0_16 finalPhase(static_cast<uint16_t>(basePhase + phaseOffsetRaw));

                    uint32_t waveRaw = clamp_frac_raw(raw(sample(finalPhase)));
                    int32_t waveCentered = (static_cast<int32_t>(waveRaw) - static_cast<int32_t>(U16_HALF)) << 1;

                    uint32_t ampRaw = clamp_frac_raw(raw(amplitude(elapsedMs)));
                    int32_t scaledWave = static_cast<int32_t>(
                        (static_cast<int64_t>(waveCentered) * static_cast<int64_t>(ampRaw)) >> 16
                    );

                    int32_t halfWave = scaledWave >> 1;
                    int32_t halfOffset = raw(offset(elapsedMs)) / 2;
                    int32_t outRaw = static_cast<int32_t>(U16_HALF) + halfWave + halfOffset;
                    return clamp01(SFracQ0_16(outRaw));
                }
            );
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
        return SFracQ0_16Signal(SignalKind::PERIODIC, [value](TimeMillis) { return value; });
    }

    SFracQ0_16Signal constant(FracQ0_16 value) {
        return SFracQ0_16Signal(
            SignalKind::PERIODIC,
            [value](TimeMillis) { return SFracQ0_16(raw(value)); }
        );
    }

    SFracQ0_16Signal cFrac(int32_t value) {
        int64_t rawValue = (static_cast<int64_t>(value) * Q0_16_ONE) / 100LL;
        if (rawValue > std::numeric_limits<int32_t>::max()) rawValue = std::numeric_limits<int32_t>::max();
        if (rawValue < std::numeric_limits<int32_t>::min()) rawValue = std::numeric_limits<int32_t>::min();
        return constant(SFracQ0_16(static_cast<int32_t>(rawValue)));
    }

    SFracQ0_16Signal cPerMil(int32_t value) {
        int64_t signedValue = value;
        bool isNegative = signedValue < 0;
        int64_t absValue = isNegative ? -signedValue : signedValue;
        if (absValue > UINT16_MAX) absValue = UINT16_MAX;
        uint16_t perMilValue = static_cast<uint16_t>(absValue);

        if (isNegative) {
            return constant(sPerMil(perMilValue));
        }
        return constant(SFracQ0_16(raw(perMil(perMilValue))));
    }

    SFracQ0_16Signal cRandom() {
        return constant(SFracQ0_16(random16()));
    }

    SFracQ0_16Signal linear(TimeMillis duration, LoopMode loopMode) {
        return createAperiodicSignal(duration, loopMode, [duration](TimeMillis t) {
            return SFracQ0_16(raw(timeToProgress(t, duration)));
        });
    }

    SFracQ0_16Signal quadraticIn(TimeMillis duration, LoopMode loopMode) {
        return createAperiodicSignal(duration, loopMode, [duration](TimeMillis t) {
            uint32_t p = raw(timeToProgress(t, duration));
            uint32_t result = (p * p) >> 16;
            return SFracQ0_16(static_cast<int32_t>(result));
        });
    }

    SFracQ0_16Signal quadraticOut(TimeMillis duration, LoopMode loopMode) {
        return createAperiodicSignal(duration, loopMode, [duration](TimeMillis t) {
            uint32_t p = raw(timeToProgress(t, duration));
            uint32_t inv = 0xFFFFu - p;
            uint32_t result = 0xFFFFu - ((inv * inv) >> 16);
            return SFracQ0_16(static_cast<int32_t>(result));
        });
    }

    SFracQ0_16Signal quadraticInOut(TimeMillis duration, LoopMode loopMode) {
        return createAperiodicSignal(duration, loopMode, [duration](TimeMillis t) {
            uint32_t p = raw(timeToProgress(t, duration));
            if (p < 0x8000u) {
                uint32_t result = (p * p) >> 15;
                return SFracQ0_16(static_cast<int32_t>(result));
            }
            uint32_t inv = 0xFFFFu - p;
            uint32_t tail = (inv * inv) >> 15;
            return SFracQ0_16(static_cast<int32_t>(0xFFFFu - tail));
        });
    }

    SFracQ0_16Signal noise(
        SFracQ0_16Signal speed,
        SFracQ0_16Signal amplitude,
        SFracQ0_16Signal offset,
        SFracQ0_16Signal phaseOffset
    ) {
        return createPeriodicSignal(
            std::move(speed),
            std::move(amplitude),
            std::move(offset),
            std::move(phaseOffset),
            sampleNoise()
        );
    }

    SFracQ0_16Signal sine(
        SFracQ0_16Signal speed,
        SFracQ0_16Signal amplitude,
        SFracQ0_16Signal offset,
        SFracQ0_16Signal phaseOffset
    ) {
        return createPeriodicSignal(
            std::move(speed),
            std::move(amplitude),
            std::move(offset),
            std::move(phaseOffset),
            sampleSine()
        );
    }

    SFracQ0_16Signal scale(SFracQ0_16Signal signal, FracQ0_16 factor) {
        if (!signal) return signal;

        auto waveform = [signal = std::move(signal), factor](TimeMillis elapsedMs) mutable {
            return mulSFracSat(signal(elapsedMs), SFracQ0_16(raw(factor)));
        };

        if (signal.kind() == SignalKind::APERIODIC) {
            return SFracQ0_16Signal(
                SignalKind::APERIODIC,
                signal.loopMode(),
                signal.duration(),
                std::move(waveform)
            );
        }
        return SFracQ0_16Signal(SignalKind::PERIODIC, std::move(waveform));
    }

    UVSignal constantUV(UV value) {
        return UVSignal([value](FracQ0_16, TimeMillis) { return value; });
    }

    UVSignal uvSignal(SFracQ0_16Signal u, SFracQ0_16Signal v) {
        return UVSignal([u = std::move(u), v = std::move(v)](FracQ0_16, TimeMillis elapsedMs) mutable {
            return UV(
                FracQ16_16(raw(u(elapsedMs))),
                FracQ16_16(raw(v(elapsedMs)))
            );
        }, true);
    }

    UVSignal uv(SFracQ0_16Signal signal, UV min, UV max) {
        UVRange range(min, max);
        auto mapped = range.mapSignal(std::move(signal));
        return UVSignal([mapped = std::move(mapped)](FracQ0_16 progress, TimeMillis elapsedMs) mutable {
            return mapped(progress, elapsedMs).get();
        }, true);
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
        uint32_t maxDepth = (max > UINT32_MAX) ? UINT32_MAX : static_cast<uint32_t>(max);
        return depth(std::move(signal), LinearRange(offset, maxDepth));
    }
}
