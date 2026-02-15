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
        const AngleRange &phaseRange() {
            static const AngleRange range{f16(0), f16(F16_MAX)};
            return range;
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
                    return speed.sample(bipolarRange(), elapsedMs);
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
                    
                    f16 pOff = phaseOffset.sample(phaseRange(), elapsedMs);
                    f16 finalPhase(raw(phase) + raw(pOff));

                    sf16 wave = sample(finalPhase);
                    sf16 amp = amplitude.sample(magnitudeRange(), elapsedMs);
                    sf16 off = offset.sample(bipolarRange(), elapsedMs);

                    int64_t res = (static_cast<int64_t>(raw(wave)) * static_cast<int64_t>(raw(amp)) + (1u << 15)) >> 16;
                    res += raw(off);
                    
                    return clampSf16Sat(res);
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
        return constant(sPerMil(value));
    }

    Sf16Signal cPerMil(uint16_t value) {
        return constant(perMil(value));
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
            uint32_t result = (static_cast<uint64_t>(p) * p + (1u << 15)) >> 16;
            return toSigned(f16(result));
        });
    }

    Sf16Signal quadraticOut(TimeMillis duration, LoopMode loopMode) {
        return createAperiodicSignal(duration, loopMode, [duration](TimeMillis t) {
            uint32_t p = raw(timeToProgress(t, duration));
            uint32_t inv = 0xFFFFu - p;
            uint32_t result = 0xFFFFu - ((static_cast<uint64_t>(inv) * inv + (1u << 15)) >> 16);
            return toSigned(f16(result));
        });
    }

    Sf16Signal quadraticInOut(TimeMillis duration, LoopMode loopMode) {
        return createAperiodicSignal(duration, loopMode, [duration](TimeMillis t) {
            uint32_t p = raw(timeToProgress(t, duration));
            if (p < 0x8000u) {
                uint32_t result = (static_cast<uint64_t>(p) * p + (1u << 14)) >> 15;
                return toSigned(f16(result));
            }
            uint32_t inv = 0xFFFFu - p;
            uint32_t tail = (static_cast<uint64_t>(inv) * inv + (1u << 14)) >> 15;
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

    Sf16Signal triangle(
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
            sampleTriangle()
        );
    }

    Sf16Signal square(
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
            sampleSquare()
        );
    }

    Sf16Signal sawtooth(
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
            sampleSawtooth()
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
