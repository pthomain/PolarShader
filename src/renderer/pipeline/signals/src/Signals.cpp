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
    const MagnitudeRange<s0x16> &magnitudeRange() {
        static const MagnitudeRange range{s0x16(0), s0x16(S0X16_MAX)};
        return range;
    }

    const BipolarRange<s0x16> &bipolarRange() {
        static const BipolarRange range{s0x16(S0X16_MIN), s0x16(S0X16_MAX)};
        return range;
    }

    namespace {
        S0x16Signal withBounds(S0x16Signal signal, S0x16Signal floor, S0x16Signal ceiling) {
            return smap(std::move(signal), std::move(floor), std::move(ceiling));
        }

        // Interpret the raw signed turn value as a wrapped 16-bit phase offset.
        u0x16 wrapPhaseOffset(s0x16 phaseOffset) {
            return u0x16(static_cast<uint16_t>(raw(phaseOffset)));
        }

        s0x16 resolveNoisePhaseOffset(s0x16 phaseOffset) {
            return raw(phaseOffset) == 0 ? randomPhaseOffset() : phaseOffset;
        }

        u0x16 timeToProgress(TimeMillis t, TimeMillis duration) {
            if (duration == 0) return u0x16(0);
            if (t >= duration) t = duration;
            uint64_t scaled = (static_cast<uint64_t>(t) * UINT16_MAX) / duration;
            if (scaled > UINT16_MAX) scaled = UINT16_MAX;
            return u0x16(static_cast<uint16_t>(scaled));
        }

        S0x16Signal createAperiodicSignal(
            TimeMillis duration,
            LoopMode loopMode,
            Waveform waveform
        ) {
            return S0x16Signal(SignalKind::APERIODIC, loopMode, duration, std::move(waveform));
        }

        S0x16Signal createPeriodicSignal(
            S0x16Signal phaseVelocity,
            s0x16 phaseOffset,
            SampleSignal sample
        ) {
            PhaseAccumulator acc(
                [phaseVelocity = std::move(phaseVelocity)](TimeMillis elapsedMs) mutable -> s0x16 {
                    return phaseVelocity.sample(magnitudeRange(), elapsedMs);
                },
                wrapPhaseOffset(phaseOffset)
            );

            return S0x16Signal(
                SignalKind::PERIODIC,
                [
                    acc = std::move(acc),
                    sample = std::move(sample)
                ](TimeMillis elapsedMs) mutable -> s0x16 {
                    return sample(acc.advance(elapsedMs));
                }
            );
        }

        S0x16Signal constantRaw(int32_t value) {
            return S0x16Signal(
                SignalKind::PERIODIC,
                [value](TimeMillis) { return s0x16(value); }
            );
        }
    }

    S0x16Signal constant(uint16_t offsetPerMil) {
        if (offsetPerMil > 1000) offsetPerMil = 1000;
        uint32_t uRaw = (static_cast<uint32_t>(offsetPerMil) * 0xFFFFu) / 1000u;
        return constant(u0x16(static_cast<uint16_t>(uRaw)));
    }

    S0x16Signal constant(s0x16 value) {
        return constantRaw(raw(value));
    }

    S0x16Signal constant(u0x16 value) {
        return constant(toSigned(value));
    }

    S0x16Signal cRandom() {
        return constant(u0x16(random16()));
    }

    s0x16 randomPhaseOffset() {
        return s0x16(random16());
    }

    S0x16Signal linear(TimeMillis duration, LoopMode loopMode) {
        return createAperiodicSignal(duration, loopMode, [duration](TimeMillis t) {
            uint32_t progressRaw = raw(timeToProgress(t, duration));
            return toSigned(u0x16(progressRaw));
        });
    }

    S0x16Signal quadraticIn(TimeMillis duration, LoopMode loopMode) {
        return createAperiodicSignal(duration, loopMode, [duration](TimeMillis t) {
            uint32_t p = raw(timeToProgress(t, duration));
            uint32_t result = (static_cast<uint64_t>(p) * p + (1u << 15)) >> 16;
            return toSigned(u0x16(result));
        });
    }

    S0x16Signal quadraticOut(TimeMillis duration, LoopMode loopMode) {
        return createAperiodicSignal(duration, loopMode, [duration](TimeMillis t) {
            uint32_t p = raw(timeToProgress(t, duration));
            uint32_t inv = 0xFFFFu - p;
            uint32_t result = 0xFFFFu - ((static_cast<uint64_t>(inv) * inv + (1u << 15)) >> 16);
            return toSigned(u0x16(result));
        });
    }

    S0x16Signal quadraticInOut(TimeMillis duration, LoopMode loopMode) {
        return createAperiodicSignal(duration, loopMode, [duration](TimeMillis t) {
            uint32_t p = raw(timeToProgress(t, duration));
            if (p < 0x8000u) {
                uint32_t result = (static_cast<uint64_t>(p) * p + (1u << 14)) >> 15;
                return toSigned(u0x16(result));
            }
            uint32_t inv = 0xFFFFu - p;
            uint32_t tail = (static_cast<uint64_t>(inv) * inv + (1u << 14)) >> 15;
            return toSigned(u0x16(0xFFFFu - tail));
        });
    }

    S0x16Signal noise(
        S0x16Signal phaseVelocity,
        s0x16 phaseOffset,
        TimeMillis loopPeriodMs
    ) {
        phaseOffset = resolveNoisePhaseOffset(phaseOffset);
        const uint32_t phaseOffsetRaw = static_cast<uint32_t>(raw(wrapPhaseOffset(phaseOffset))) << 16;

        if (loopPeriodMs > 0) {
            // Seamless loop via a two-path cross-dissolve (same technique as the
            // looping noise pattern): out(phi) = lerp(noise(cA), noise(cB), w(phi))
            // with cA = base + phi*span, cB = cA - span, and a monotonic 0->1
            // weight, so out(0) == out(1). The random phaseOffset (base) gives each
            // looping signal its own path through the field.
            return S0x16Signal(
                SignalKind::PERIODIC,
                [
                    phaseVelocity = std::move(phaseVelocity),
                    phaseOffsetRaw,
                    loopPeriodMs,
                    sampler = sampleNoise32(),
                    span = static_cast<uint32_t>(0),
                    spanReady = false
                ](TimeMillis elapsedMs) mutable -> s0x16 {
                    if (!spanReady) {
                        // Sample velocity ONCE at epoch 0 so the span (hence the
                        // seam) is a fixed function of absolute time, matching the
                        // pattern's loop. At phaseVelocity 1.0 the coordinate
                        // advances by 1 per millisecond, so one period spans P*v.
                        const uint32_t vel = static_cast<uint32_t>(
                            raw(phaseVelocity.sample(magnitudeRange(), 0u)));
                        span = static_cast<uint32_t>(
                            (static_cast<uint64_t>(loopPeriodMs) * vel) >> 16);
                        spanReady = true;
                    }

                    const uint16_t phi_raw = static_cast<uint16_t>(
                        static_cast<uint64_t>(elapsedMs % loopPeriodMs) * 65535u / loopPeriodMs);
                    const uint32_t coordA = phaseOffsetRaw +
                        static_cast<uint32_t>((static_cast<uint64_t>(phi_raw) * span) >> 16);
                    const uint32_t coordB = coordA - span;

                    int32_t w = 32767 - static_cast<int32_t>(cos16(static_cast<uint16_t>(phi_raw >> 1)));
                    if (w < 0) w = 0;
                    if (w > 65535) w = 65535;

                    const int32_t a = raw(sampler(coordA));
                    const int32_t b = raw(sampler(coordB));
                    const int32_t out = a + static_cast<int32_t>(
                        (static_cast<int64_t>(b - a) * w) >> 16);
                    return s0x16(out);
                }
            );
        }

        return S0x16Signal(
            SignalKind::PERIODIC,
            [
                phaseVelocity = std::move(phaseVelocity),
                phaseOffsetRaw,
                sampler = sampleNoise32()
            ](TimeMillis elapsedMs) mutable -> s0x16 {
                s0x16 s = phaseVelocity.sample(magnitudeRange(), elapsedMs);

                // Noise coordinate is driven by elapsed time scaled by the sampled phase velocity.
                // At phaseVelocity 1.0 (65536), the coordinate advances by 1 per millisecond.
                uint64_t coord64 = (static_cast<uint64_t>(elapsedMs) * static_cast<uint32_t>(raw(s))) >> 16;
                uint32_t coord = static_cast<uint32_t>(coord64);
                return sampler(coord + phaseOffsetRaw);
            }
        );
    }

    S0x16Signal noise(
        S0x16Signal phaseVelocity,
        S0x16Signal floor,
        S0x16Signal ceiling,
        TimeMillis loopPeriodMs
    ) {
        return withBounds(
            noise(std::move(phaseVelocity), randomPhaseOffset(), loopPeriodMs),
            std::move(floor),
            std::move(ceiling)
        );
    }

    S0x16Signal noise(
        S0x16Signal phaseVelocity,
        s0x16 phaseOffset,
        S0x16Signal floor,
        S0x16Signal ceiling,
        TimeMillis loopPeriodMs
    ) {
        return withBounds(
            noise(std::move(phaseVelocity), phaseOffset, loopPeriodMs),
            std::move(floor),
            std::move(ceiling)
        );
    }

    S0x16Signal sine(
        S0x16Signal phaseVelocity,
        s0x16 phaseOffset
    ) {
        return createPeriodicSignal(
            std::move(phaseVelocity),
            phaseOffset,
            sampleSine()
        );
    }

    S0x16Signal sine(
        S0x16Signal phaseVelocity,
        S0x16Signal floor,
        S0x16Signal ceiling
    ) {
        return withBounds(
            sine(std::move(phaseVelocity)),
            std::move(floor),
            std::move(ceiling)
        );
    }

    S0x16Signal sine(
        S0x16Signal phaseVelocity,
        s0x16 phaseOffset,
        S0x16Signal floor,
        S0x16Signal ceiling
    ) {
        return withBounds(
            sine(std::move(phaseVelocity), phaseOffset),
            std::move(floor),
            std::move(ceiling)
        );
    }

    S0x16Signal triangle(
        S0x16Signal phaseVelocity,
        s0x16 phaseOffset
    ) {
        return createPeriodicSignal(
            std::move(phaseVelocity),
            phaseOffset,
            sampleTriangle()
        );
    }

    S0x16Signal triangle(
        S0x16Signal phaseVelocity,
        S0x16Signal floor,
        S0x16Signal ceiling
    ) {
        return withBounds(
            triangle(std::move(phaseVelocity)),
            std::move(floor),
            std::move(ceiling)
        );
    }

    S0x16Signal triangle(
        S0x16Signal phaseVelocity,
        s0x16 phaseOffset,
        S0x16Signal floor,
        S0x16Signal ceiling
    ) {
        return withBounds(
            triangle(std::move(phaseVelocity), phaseOffset),
            std::move(floor),
            std::move(ceiling)
        );
    }

    S0x16Signal square(
        S0x16Signal phaseVelocity,
        s0x16 phaseOffset
    ) {
        return createPeriodicSignal(
            std::move(phaseVelocity),
            phaseOffset,
            sampleSquare()
        );
    }

    S0x16Signal square(
        S0x16Signal phaseVelocity,
        S0x16Signal floor,
        S0x16Signal ceiling
    ) {
        return withBounds(
            square(std::move(phaseVelocity)),
            std::move(floor),
            std::move(ceiling)
        );
    }

    S0x16Signal square(
        S0x16Signal phaseVelocity,
        s0x16 phaseOffset,
        S0x16Signal floor,
        S0x16Signal ceiling
    ) {
        return withBounds(
            square(std::move(phaseVelocity), phaseOffset),
            std::move(floor),
            std::move(ceiling)
        );
    }

    S0x16Signal sawtooth(
        S0x16Signal phaseVelocity,
        s0x16 phaseOffset
    ) {
        return createPeriodicSignal(
            std::move(phaseVelocity),
            phaseOffset,
            sampleSawtooth()
        );
    }

    S0x16Signal sawtooth(
        S0x16Signal phaseVelocity,
        S0x16Signal floor,
        S0x16Signal ceiling
    ) {
        return withBounds(
            sawtooth(std::move(phaseVelocity)),
            std::move(floor),
            std::move(ceiling)
        );
    }

    S0x16Signal sawtooth(
        S0x16Signal phaseVelocity,
        s0x16 phaseOffset,
        S0x16Signal floor,
        S0x16Signal ceiling
    ) {
        return withBounds(
            sawtooth(std::move(phaseVelocity), phaseOffset),
            std::move(floor),
            std::move(ceiling)
        );
    }

    S0x16Signal scale(S0x16Signal signal, u0x16 factor) {
        if (!signal) return signal;

        auto waveform = [signal = std::move(signal), factor](TimeMillis elapsedMs) mutable {
            return mulS0x16Sat(signal.sample(bipolarRange(), elapsedMs), s0x16(raw(factor)));
        };

        if (signal.kind() == SignalKind::APERIODIC) {
            return S0x16Signal(
                SignalKind::APERIODIC,
                signal.loopMode(),
                signal.duration(),
                std::move(waveform)
            );
        }
        return S0x16Signal(SignalKind::PERIODIC, std::move(waveform));
    }

    S0x16Signal smap(S0x16Signal signal, S0x16Signal floor, S0x16Signal ceiling) {
        if (!signal) return signal;
        if (!floor) floor = constant(0);
        if (!ceiling) ceiling = constant(1000);

        auto waveform = [
                    signal = std::move(signal),
                    floor = std::move(floor),
                    ceiling = std::move(ceiling)
                ](TimeMillis elapsedMs) mutable -> s0x16 {
            // Bounds live in the unipolar domain and may animate independently over time.
            uint32_t floorRaw = static_cast<uint32_t>(raw(floor.sample(magnitudeRange(), elapsedMs)));
            uint32_t ceilingRaw = static_cast<uint32_t>(raw(ceiling.sample(magnitudeRange(), elapsedMs)));
            if (floorRaw > ceilingRaw) {
                std::swap(floorRaw, ceilingRaw);
            }

            const uint32_t sourceRaw = raw(toUnsignedClamped(signal.sample(bipolarRange(), elapsedMs)));
            const uint32_t span = ceilingRaw - floorRaw;
            const uint32_t mappedRaw = floorRaw + ((span * sourceRaw + (1u << 15)) >> 16);
            return toSigned(u0x16(static_cast<uint16_t>(mappedRaw)));
        };

        if (signal.kind() == SignalKind::APERIODIC) {
            return S0x16Signal(
                SignalKind::APERIODIC,
                signal.loopMode(),
                signal.duration(),
                std::move(waveform)
            );
        }
        return S0x16Signal(SignalKind::PERIODIC, std::move(waveform));
    }

    UVSignal constantUV(UV value) {
        return UVSignal([value](u0x16, TimeMillis) { return value; });
    }

    UVSignal uvSignal(S0x16Signal u, S0x16Signal v) {
        return UVSignal([u = std::move(u), v = std::move(v)](u0x16, TimeMillis elapsedMs) mutable {
            return UV(
                fl::s16x16::from_raw(raw(u.sample(magnitudeRange(), elapsedMs))),
                fl::s16x16::from_raw(raw(v.sample(magnitudeRange(), elapsedMs)))
            );
        });
    }

    UVSignal uvInRange(S0x16Signal signal, UV min, UV max) {
        UVRange range(min, max);
        return UVSignal([signal = std::move(signal), range](u0x16, TimeMillis elapsedMs) mutable {
            return signal.sample(range, elapsedMs);
        });
    }

    UVSignal uv(S0x16Signal signal, UV min, UV max) {
        return uvInRange(std::move(signal), min, max);
    }
}
