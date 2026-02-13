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
#include "renderer/pipeline/signals/accumulators/Accumulators.h"
#include "renderer/pipeline/signals/SignalSamplers.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"
#include "renderer/pipeline/signals/ranges/UVRange.h"
#include <cstdint>
#include <utility>

namespace PolarShader {
    const MagnitudeRange<sf16> &magnitudeRange() {
        static const MagnitudeRange range{sf16(0), sf16(SF16_MAX)};
        return range;
    }

    const BipolarRange<sf16> &bipolarRange() {
        static const BipolarRange range{sf16(SF16_MIN), sf16(SF16_MAX)};
        return range;
    }

    namespace {
        int32_t sampleSpeedRaw(Sf16Signal &signal, TimeMillis elapsedMs) {
            return raw(signal.sample(magnitudeRange(), elapsedMs));
        }

        f16 timeToProgress(TimeMillis t, TimeMillis duration) {
            if (duration == 0) return f16(0);
            if (t >= duration) t = duration;
            uint64_t scaled = (static_cast<uint64_t>(t) * UINT16_MAX) / duration;
            if (scaled > UINT16_MAX) scaled = UINT16_MAX;
            return f16(static_cast<uint16_t>(scaled));
        }

        Sf16Signal createAperiodicSignal(
            TimeMillis duration,
            LoopMode loopMode,
            Waveform waveform
        ) {
            return Sf16Signal(SignalKind::APERIODIC, loopMode, duration, std::move(waveform));
        }

        Sf16Signal createPeriodicSignal(
            Sf16Signal speed,
            Sf16Signal amplitude,
            Sf16Signal offset,
            Sf16Signal phaseOffset,
            SampleSignal sample
        ) {
            PhaseAccumulator acc(
                [speed = std::move(speed)](TimeMillis elapsedMs) mutable -> sf16 {
                    return sf16(sampleSpeedRaw(speed, elapsedMs));
                }
            );

            return Sf16Signal(
                SignalKind::PERIODIC,
                [
                    acc = std::move(acc),
                    amplitude = std::move(amplitude),
                    offset = std::move(offset),
                    phaseOffset = std::move(phaseOffset),
                    sample = std::move(sample)
                ](TimeMillis elapsedMs) mutable -> sf16 {
                    f16 phase = acc.advance(elapsedMs);
                    uint32_t basePhase = raw(phase);

                    uint32_t phaseOffsetRaw = raw(phaseOffset.sample(magnitudeRange(), elapsedMs));
                    f16 finalPhase(static_cast<uint16_t>(basePhase + phaseOffsetRaw));

                    int32_t waveSignedRaw = raw(sample(finalPhase));
                    uint32_t waveUnitRaw = raw(toUnsigned(sf16(waveSignedRaw)));

                    uint32_t ampRaw = static_cast<uint32_t>(raw(amplitude.sample(magnitudeRange(), elapsedMs)));
                    int32_t waveCentered = (static_cast<int32_t>(waveUnitRaw) - static_cast<int32_t>(U16_HALF)) << 1;
                    int32_t scaledWave = static_cast<int32_t>(
                        (static_cast<int64_t>(waveCentered) * static_cast<int64_t>(ampRaw)) >> 16
                    );

                    int32_t halfWave = scaledWave >> 1;
                    int32_t halfOffset = raw(offset.sample(magnitudeRange(), elapsedMs)) >> 1;
                    int32_t outUnitRaw = static_cast<int32_t>(U16_HALF) + halfWave + halfOffset;
                    if (outUnitRaw < 0) outUnitRaw = 0;
                    if (outUnitRaw > SF16_MAX) outUnitRaw = SF16_MAX;

                    return toSigned(f16(static_cast<uint16_t>(outUnitRaw)));
                }
            );
        }

        Sf16Signal constantRaw(int32_t value) {
            return Sf16Signal(
                SignalKind::PERIODIC,
                [value](TimeMillis) { return sf16(value); }
            );
        }
    }

    Sf16Signal floor() {
        return constantRaw(SF16_MIN);
    }

    Sf16Signal midPoint() {
        return constantRaw(0);
    }

    Sf16Signal ceiling() {
        return constantRaw(SF16_MAX);
    }

    Sf16Signal constant(sf16 value) {
        return constantRaw(raw(value));
    }

    Sf16Signal constant(f16 value) {
        return constantRaw(raw(toSigned(value)));
    }

    Sf16Signal csPerMil(int16_t value) {
        int32_t clamped = value;
        if (clamped > 1000) clamped = 1000;
        if (clamped < -1000) clamped = -1000;

        int32_t raw_value = (clamped * SF16_ONE) / 1000;
        return constantRaw(raw_value);
    }

    Sf16Signal cPerMil(uint16_t value) {
        if (value > 1000u) value = 1000u;
        int32_t mapped = static_cast<int32_t>(value) * 2 - 1000;
        return csPerMil(static_cast<int16_t>(mapped));
    }

    Sf16Signal cRandom() {
        return constantRaw(raw(toSigned(f16(random16()))));
    }

    Sf16Signal linear(TimeMillis duration, LoopMode loopMode) {
        return createAperiodicSignal(duration, loopMode, [duration](TimeMillis t) {
            uint32_t progressRaw = raw(timeToProgress(t, duration));
            return toSigned(f16(progressRaw));
        });
    }

    Sf16Signal quadraticIn(TimeMillis duration, LoopMode loopMode) {
        return createAperiodicSignal(duration, loopMode, [duration](TimeMillis t) {
            uint32_t p = raw(timeToProgress(t, duration));
            uint32_t result = (p * p) >> 16;
            return toSigned(f16(result));
        });
    }

    Sf16Signal quadraticOut(TimeMillis duration, LoopMode loopMode) {
        return createAperiodicSignal(duration, loopMode, [duration](TimeMillis t) {
            uint32_t p = raw(timeToProgress(t, duration));
            uint32_t inv = 0xFFFFu - p;
            uint32_t result = 0xFFFFu - ((inv * inv) >> 16);
            return toSigned(f16(result));
        });
    }

    Sf16Signal quadraticInOut(TimeMillis duration, LoopMode loopMode) {
        return createAperiodicSignal(duration, loopMode, [duration](TimeMillis t) {
            uint32_t p = raw(timeToProgress(t, duration));
            if (p < 0x8000u) {
                uint32_t result = (p * p) >> 15;
                return toSigned(f16(result));
            }
            uint32_t inv = 0xFFFFu - p;
            uint32_t tail = (inv * inv) >> 15;
            return toSigned(f16(0xFFFFu - tail));
        });
    }

    Sf16Signal noise(
        Sf16Signal speed,
        Sf16Signal amplitude,
        Sf16Signal offset,
        Sf16Signal phaseOffset
    ) {
        return createPeriodicSignal(
            std::move(speed),
            std::move(amplitude),
            std::move(offset),
            std::move(phaseOffset),
            sampleNoise()
        );
    }

    Sf16Signal sine(
        Sf16Signal speed,
        Sf16Signal amplitude,
        Sf16Signal offset,
        Sf16Signal phaseOffset
    ) {
        return createPeriodicSignal(
            std::move(speed),
            std::move(amplitude),
            std::move(offset),
            std::move(phaseOffset),
            sampleSine()
        );
    }

    Sf16Signal scale(Sf16Signal signal, f16 factor) {
        if (!signal) return signal;

        auto waveform = [signal = std::move(signal), factor](TimeMillis elapsedMs) mutable {
            return mulSf16Sat(signal.sample(bipolarRange(), elapsedMs), sf16(raw(factor)));
        };

        if (signal.kind() == SignalKind::APERIODIC) {
            return Sf16Signal(
                SignalKind::APERIODIC,
                signal.loopMode(),
                signal.duration(),
                std::move(waveform)
            );
        }
        return Sf16Signal(SignalKind::PERIODIC, std::move(waveform));
    }

    UVSignal constantUV(UV value) {
        return UVSignal([value](f16, TimeMillis) { return value; });
    }

    UVSignal uvSignal(Sf16Signal u, Sf16Signal v) {
        return UVSignal([u = std::move(u), v = std::move(v)](f16, TimeMillis elapsedMs) mutable {
            return UV(
                sr16(raw(u.sample(magnitudeRange(), elapsedMs))),
                sr16(raw(v.sample(magnitudeRange(), elapsedMs)))
            );
        });
    }

    UVSignal uvInRange(Sf16Signal signal, UV min, UV max) {
        UVRange range(min, max);
        return UVSignal([signal = std::move(signal), range](f16, TimeMillis elapsedMs) mutable {
            return signal.sample(range, elapsedMs);
        });
    }

    UVSignal uv(Sf16Signal signal, UV min, UV max) {
        return uvInRange(std::move(signal), min, max);
    }

    DepthSignal constantDepth(uint32_t value) {
        return [value](f16, TimeMillis) { return value; };
    }

    DepthSignal depth(
        Sf16Signal signal,
        MagnitudeRange<uint32_t> range
    ) {
        return [signal = std::move(signal), range](f16, TimeMillis elapsedMs) mutable -> uint32_t {
            return signal.sample(range, elapsedMs);
        };
    }

    DepthSignal depth(
        Sf16Signal signal,
        uint32_t scale,
        uint32_t offset
    ) {
        uint64_t max = static_cast<uint64_t>(offset) + static_cast<uint64_t>(scale);
        uint32_t maxDepth = (max > UINT32_MAX) ? UINT32_MAX : static_cast<uint32_t>(max);
        return depth(std::move(signal), MagnitudeRange(offset, maxDepth));
    }
}
