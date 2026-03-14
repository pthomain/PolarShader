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
        Sf16Signal withBounds(Sf16Signal signal, Sf16Signal floor, Sf16Signal ceiling) {
            return smap(std::move(signal), std::move(floor), std::move(ceiling));
        }

        // Interpret the raw signed turn value as a wrapped 16-bit phase offset.
        f16 wrapPhaseOffset(sf16 phaseOffset) {
            return f16(static_cast<uint16_t>(raw(phaseOffset)));
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
            Sf16Signal phaseVelocity,
            sf16 phaseOffset,
            SampleSignal sample
        ) {
            PhaseAccumulator acc(
                [phaseVelocity = std::move(phaseVelocity)](TimeMillis elapsedMs) mutable -> sf16 {
                    return phaseVelocity.sample(bipolarRange(), elapsedMs);
                },
                wrapPhaseOffset(phaseOffset)
            );

            return Sf16Signal(
                SignalKind::PERIODIC,
                [
                    acc = std::move(acc),
                    sample = std::move(sample)
                ](TimeMillis elapsedMs) mutable -> sf16 {
                    return sample(acc.advance(elapsedMs));
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

    Sf16Signal constant(uint16_t offsetPerMil) {
        if (offsetPerMil > 1000) offsetPerMil = 1000;
        uint32_t uRaw = (static_cast<uint32_t>(offsetPerMil) * 0xFFFFu) / 1000u;
        return constant(f16(static_cast<uint16_t>(uRaw)));
    }

    Sf16Signal constant(sf16 value) {
        return constantRaw(raw(value));
    }

    Sf16Signal constant(f16 value) {
        return constant(toSigned(value));
    }

    Sf16Signal cRandom() {
        return constant(f16(random16()));
    }

    sf16 randomPhaseOffset() {
        return sf16(random16());
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
        Sf16Signal phaseVelocity,
        sf16 phaseOffset
    ) {
        const uint32_t phaseOffsetRaw = static_cast<uint32_t>(raw(wrapPhaseOffset(phaseOffset))) << 16;

        return Sf16Signal(
            SignalKind::PERIODIC,
            [
                phaseVelocity = std::move(phaseVelocity),
                phaseOffsetRaw,
                sampler = sampleNoise32()
            ](TimeMillis elapsedMs) mutable -> sf16 {
                sf16 s = phaseVelocity.sample(magnitudeRange(), elapsedMs);

                // Noise coordinate is driven by elapsed time scaled by the sampled phase velocity.
                // At phaseVelocity 1.0 (65536), the coordinate advances by 1 per millisecond.
                uint64_t coord64 = (static_cast<uint64_t>(elapsedMs) * static_cast<uint32_t>(raw(s))) >> 16;
                uint32_t coord = static_cast<uint32_t>(coord64);
                return sampler(coord + phaseOffsetRaw);
            }
        );
    }

    Sf16Signal noise(
        Sf16Signal phaseVelocity,
        Sf16Signal floor,
        Sf16Signal ceiling
    ) {
        return withBounds(
            noise(std::move(phaseVelocity)),
            std::move(floor),
            std::move(ceiling)
        );
    }

    Sf16Signal noise(
        Sf16Signal phaseVelocity,
        sf16 phaseOffset,
        Sf16Signal floor,
        Sf16Signal ceiling
    ) {
        return withBounds(
            noise(std::move(phaseVelocity), phaseOffset),
            std::move(floor),
            std::move(ceiling)
        );
    }

    Sf16Signal sine(
        Sf16Signal phaseVelocity,
        sf16 phaseOffset
    ) {
        return createPeriodicSignal(
            std::move(phaseVelocity),
            phaseOffset,
            sampleSine()
        );
    }

    Sf16Signal sine(
        Sf16Signal phaseVelocity,
        Sf16Signal floor,
        Sf16Signal ceiling
    ) {
        return withBounds(
            sine(std::move(phaseVelocity)),
            std::move(floor),
            std::move(ceiling)
        );
    }

    Sf16Signal sine(
        Sf16Signal phaseVelocity,
        sf16 phaseOffset,
        Sf16Signal floor,
        Sf16Signal ceiling
    ) {
        return withBounds(
            sine(std::move(phaseVelocity), phaseOffset),
            std::move(floor),
            std::move(ceiling)
        );
    }

    Sf16Signal triangle(
        Sf16Signal phaseVelocity,
        sf16 phaseOffset
    ) {
        return createPeriodicSignal(
            std::move(phaseVelocity),
            phaseOffset,
            sampleTriangle()
        );
    }

    Sf16Signal triangle(
        Sf16Signal phaseVelocity,
        Sf16Signal floor,
        Sf16Signal ceiling
    ) {
        return withBounds(
            triangle(std::move(phaseVelocity)),
            std::move(floor),
            std::move(ceiling)
        );
    }

    Sf16Signal triangle(
        Sf16Signal phaseVelocity,
        sf16 phaseOffset,
        Sf16Signal floor,
        Sf16Signal ceiling
    ) {
        return withBounds(
            triangle(std::move(phaseVelocity), phaseOffset),
            std::move(floor),
            std::move(ceiling)
        );
    }

    Sf16Signal square(
        Sf16Signal phaseVelocity,
        sf16 phaseOffset
    ) {
        return createPeriodicSignal(
            std::move(phaseVelocity),
            phaseOffset,
            sampleSquare()
        );
    }

    Sf16Signal square(
        Sf16Signal phaseVelocity,
        Sf16Signal floor,
        Sf16Signal ceiling
    ) {
        return withBounds(
            square(std::move(phaseVelocity)),
            std::move(floor),
            std::move(ceiling)
        );
    }

    Sf16Signal square(
        Sf16Signal phaseVelocity,
        sf16 phaseOffset,
        Sf16Signal floor,
        Sf16Signal ceiling
    ) {
        return withBounds(
            square(std::move(phaseVelocity), phaseOffset),
            std::move(floor),
            std::move(ceiling)
        );
    }

    Sf16Signal sawtooth(
        Sf16Signal phaseVelocity,
        sf16 phaseOffset
    ) {
        return createPeriodicSignal(
            std::move(phaseVelocity),
            phaseOffset,
            sampleSawtooth()
        );
    }

    Sf16Signal sawtooth(
        Sf16Signal phaseVelocity,
        Sf16Signal floor,
        Sf16Signal ceiling
    ) {
        return withBounds(
            sawtooth(std::move(phaseVelocity)),
            std::move(floor),
            std::move(ceiling)
        );
    }

    Sf16Signal sawtooth(
        Sf16Signal phaseVelocity,
        sf16 phaseOffset,
        Sf16Signal floor,
        Sf16Signal ceiling
    ) {
        return withBounds(
            sawtooth(std::move(phaseVelocity), phaseOffset),
            std::move(floor),
            std::move(ceiling)
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

    Sf16Signal smap(Sf16Signal signal, Sf16Signal floor, Sf16Signal ceiling) {
        if (!signal) return signal;
        if (!floor) floor = constant(0);
        if (!ceiling) ceiling = constant(1000);

        auto waveform = [
                    signal = std::move(signal),
                    floor = std::move(floor),
                    ceiling = std::move(ceiling)
                ](TimeMillis elapsedMs) mutable -> sf16 {
            // Bounds live in the unipolar domain and may animate independently over time.
            uint32_t floorRaw = static_cast<uint32_t>(raw(floor.sample(magnitudeRange(), elapsedMs)));
            uint32_t ceilingRaw = static_cast<uint32_t>(raw(ceiling.sample(magnitudeRange(), elapsedMs)));
            if (floorRaw > ceilingRaw) {
                std::swap(floorRaw, ceilingRaw);
            }

            const uint32_t sourceRaw = raw(toUnsignedClamped(signal.sample(bipolarRange(), elapsedMs)));
            const uint32_t span = ceilingRaw - floorRaw;
            const uint32_t mappedRaw = floorRaw + ((span * sourceRaw + (1u << 15)) >> 16);
            return toSigned(f16(static_cast<uint16_t>(mappedRaw)));
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
                fl::s16x16::from_raw(raw(u.sample(magnitudeRange(), elapsedMs))),
                fl::s16x16::from_raw(raw(v.sample(magnitudeRange(), elapsedMs)))
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
}
