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
#include "renderer/pipeline/signals/ranges/LinearRange.h"
#include "renderer/pipeline/signals/ranges/UVRange.h"
#include <cstdint>
#include <limits>
#include <utility>

namespace PolarShader {
    const LinearRange<SQ0_16> &unitRange() {
        // Normalized scalar signal domain [0..1] in raw Q0.16 units.
        // Used when a caller needs the clamped scalar sample itself before custom math.
        static const LinearRange range(
            SQ0_16(0),
            SQ0_16(SQ0_16_MAX),
            RangeMappingMode::UnsignedFromSigned
        );
        return range;
    }

    const LinearRange<SQ0_16> &signedUnitRange() {
        // Signed scalar signal domain [-1..1] in raw Q0.16 units.
        static const LinearRange range{
            SQ0_16(SQ0_16_MIN),
            SQ0_16(SQ0_16_MAX),
            RangeMappingMode::SignedDirect
        };
        return range;
    }

    namespace {
        constexpr int32_t SIGNAL_SPEED_SCALE = 1;

        int32_t sampleSignedSpeedRaw(SQ0_16Signal &signal, TimeMillis elapsedMs) {
            return raw(signal.sample(signedUnitRange(), elapsedMs));
        }

        UQ0_16 timeToProgress(TimeMillis t, TimeMillis duration) {
            if (duration == 0) return UQ0_16(0);
            if (t >= duration) t = duration;
            uint64_t scaled = (static_cast<uint64_t>(t) * UINT16_MAX) / duration;
            if (scaled > UINT16_MAX) scaled = UINT16_MAX;
            return UQ0_16(static_cast<uint16_t>(scaled));
        }

        SQ0_16Signal createAperiodicSignal(
            TimeMillis duration,
            LoopMode loopMode,
            Waveform waveform
        ) {
            return SQ0_16Signal(SignalKind::APERIODIC, loopMode, duration, std::move(waveform));
        }

        SQ0_16Signal createPeriodicSignal(
            SQ0_16Signal speed,
            SQ0_16Signal amplitude,
            SQ0_16Signal offset,
            SQ0_16Signal phaseOffset,
            SampleSignal sample
        ) {
            PhaseAccumulator acc(
                [speed = std::move(speed)](TimeMillis elapsedMs) mutable -> SQ0_16 {
                    int64_t speedRaw = static_cast<int64_t>(sampleSignedSpeedRaw(speed, elapsedMs)) *
                                       SIGNAL_SPEED_SCALE;
                    if (speedRaw > std::numeric_limits<int32_t>::max()) speedRaw = std::numeric_limits<int32_t>::max();
                    if (speedRaw < std::numeric_limits<int32_t>::min()) speedRaw = std::numeric_limits<int32_t>::min();
                    return SQ0_16(static_cast<int32_t>(speedRaw));
                }
            );

            return SQ0_16Signal(
                SignalKind::PERIODIC,
                [acc = std::move(acc),
                    amplitude = std::move(amplitude),
                    offset = std::move(offset),
                    phaseOffset = std::move(phaseOffset),
                    sample = std::move(sample)](TimeMillis elapsedMs) mutable -> SQ0_16 {
                    UQ0_16 phase = acc.advance(elapsedMs);
                    uint32_t basePhase = raw(phase);
                    uint32_t phaseOffsetRaw = static_cast<uint32_t>(raw(phaseOffset.sample(unitRange(), elapsedMs)));
                    UQ0_16 finalPhase(static_cast<uint16_t>(basePhase + phaseOffsetRaw));

                    int32_t waveSignedRaw = raw(sample(finalPhase));
                    uint32_t waveUnitRaw = signed_to_unit_raw(waveSignedRaw);

                    uint32_t ampRaw = static_cast<uint32_t>(raw(amplitude.sample(unitRange(), elapsedMs)));
                    int32_t waveCentered = (static_cast<int32_t>(waveUnitRaw) - static_cast<int32_t>(U16_HALF)) << 1;
                    int32_t scaledWave = static_cast<int32_t>(
                        (static_cast<int64_t>(waveCentered) * static_cast<int64_t>(ampRaw)) >> 16
                    );

                    int32_t halfWave = scaledWave >> 1;
                    int32_t halfOffset = raw(offset.sample(unitRange(), elapsedMs)) >> 1;
                    int32_t outUnitRaw = static_cast<int32_t>(U16_HALF) + halfWave + halfOffset;
                    if (outUnitRaw < 0) outUnitRaw = 0;
                    if (outUnitRaw > SQ0_16_MAX) outUnitRaw = SQ0_16_MAX;

                    return SQ0_16(unit_to_signed_raw(static_cast<uint32_t>(outUnitRaw)));
                }
            );
        }

        SQ0_16Signal constantRaw(int32_t value) {
            return SQ0_16Signal(
                SignalKind::PERIODIC,
                [value](TimeMillis) { return SQ0_16(value); }
            );
        }
    }

    SQ0_16Signal floor() {
        return constantRaw(SQ0_16_MIN);
    }

    SQ0_16Signal midPoint() {
        return constantRaw(0);
    }

    SQ0_16Signal ceiling() {
        return constantRaw(SQ0_16_MAX);
    }

    SQ0_16Signal constant(SQ0_16 value) {
        return constantRaw(raw(value));
    }

    SQ0_16Signal constant(UQ0_16 value) {
        return constantRaw(unit_to_signed_raw(raw(value)));
    }

    SQ0_16Signal cFrac(int32_t value) {
        int64_t rawValue = (static_cast<int64_t>(value) * SQ0_16_ONE) / 100LL;
        if (rawValue > std::numeric_limits<int32_t>::max()) rawValue = std::numeric_limits<int32_t>::max();
        if (rawValue < std::numeric_limits<int32_t>::min()) rawValue = std::numeric_limits<int32_t>::min();
        return constantRaw(static_cast<int32_t>(rawValue));
    }

    SQ0_16Signal cPerMil(int32_t value) {
        int64_t signedValue = value;
        bool isNegative = signedValue < 0;
        int64_t absValue = isNegative ? -signedValue : signedValue;
        if (absValue > UINT16_MAX) absValue = UINT16_MAX;
        uint16_t perMilValue = static_cast<uint16_t>(absValue);

        if (isNegative) {
            return constant(sPerMil(perMilValue));
        }
        return constant(SQ0_16(raw(uPerMil(perMilValue))));
    }

    SQ0_16Signal cRandom() {
        return constantRaw(unit_to_signed_raw(random16()));
    }

    SQ0_16Signal linear(TimeMillis duration, LoopMode loopMode) {
        return createAperiodicSignal(duration, loopMode, [duration](TimeMillis t) {
            uint32_t progressRaw = raw(timeToProgress(t, duration));
            return SQ0_16(unit_to_signed_raw(progressRaw));
        });
    }

    SQ0_16Signal quadraticIn(TimeMillis duration, LoopMode loopMode) {
        return createAperiodicSignal(duration, loopMode, [duration](TimeMillis t) {
            uint32_t p = raw(timeToProgress(t, duration));
            uint32_t result = (p * p) >> 16;
            return SQ0_16(unit_to_signed_raw(result));
        });
    }

    SQ0_16Signal quadraticOut(TimeMillis duration, LoopMode loopMode) {
        return createAperiodicSignal(duration, loopMode, [duration](TimeMillis t) {
            uint32_t p = raw(timeToProgress(t, duration));
            uint32_t inv = 0xFFFFu - p;
            uint32_t result = 0xFFFFu - ((inv * inv) >> 16);
            return SQ0_16(unit_to_signed_raw(result));
        });
    }

    SQ0_16Signal quadraticInOut(TimeMillis duration, LoopMode loopMode) {
        return createAperiodicSignal(duration, loopMode, [duration](TimeMillis t) {
            uint32_t p = raw(timeToProgress(t, duration));
            if (p < 0x8000u) {
                uint32_t result = (p * p) >> 15;
                return SQ0_16(unit_to_signed_raw(result));
            }
            uint32_t inv = 0xFFFFu - p;
            uint32_t tail = (inv * inv) >> 15;
            return SQ0_16(unit_to_signed_raw(0xFFFFu - tail));
        });
    }

    SQ0_16Signal noise(
        SQ0_16Signal speed,
        SQ0_16Signal amplitude,
        SQ0_16Signal offset,
        SQ0_16Signal phaseOffset
    ) {
        return createPeriodicSignal(
            std::move(speed),
            std::move(amplitude),
            std::move(offset),
            std::move(phaseOffset),
            sampleNoise()
        );
    }

    SQ0_16Signal sine(
        SQ0_16Signal speed,
        SQ0_16Signal amplitude,
        SQ0_16Signal offset,
        SQ0_16Signal phaseOffset
    ) {
        return createPeriodicSignal(
            std::move(speed),
            std::move(amplitude),
            std::move(offset),
            std::move(phaseOffset),
            sampleSine()
        );
    }

    SQ0_16Signal scale(SQ0_16Signal signal, UQ0_16 factor) {
        if (!signal) return signal;

        auto waveform = [signal = std::move(signal), factor](TimeMillis elapsedMs) mutable {
            return mulSFracSat(signal.sample(signedUnitRange(), elapsedMs), SQ0_16(raw(factor)));
        };

        if (signal.kind() == SignalKind::APERIODIC) {
            return SQ0_16Signal(
                SignalKind::APERIODIC,
                signal.loopMode(),
                signal.duration(),
                std::move(waveform)
            );
        }
        return SQ0_16Signal(SignalKind::PERIODIC, std::move(waveform));
    }

    UVSignal constantUV(UV value) {
        return UVSignal([value](UQ0_16, TimeMillis) { return value; });
    }

    UVSignal uvSignal(SQ0_16Signal u, SQ0_16Signal v) {
        return UVSignal([u = std::move(u), v = std::move(v)](UQ0_16, TimeMillis elapsedMs) mutable {
            return UV(
                SQ16_16(raw(u.sample(unitRange(), elapsedMs))),
                SQ16_16(raw(v.sample(unitRange(), elapsedMs)))
            );
        });
    }

    UVSignal uvInRange(SQ0_16Signal signal, UV min, UV max) {
        UVRange range(min, max);
        return UVSignal([signal = std::move(signal), range](UQ0_16, TimeMillis elapsedMs) mutable {
            return signal.sample(range, elapsedMs);
        });
    }

    UVSignal uv(SQ0_16Signal signal, UV min, UV max) {
        return uvInRange(std::move(signal), min, max);
    }

    DepthSignal constantDepth(uint32_t value) {
        return [value](UQ0_16, TimeMillis) { return value; };
    }

    DepthSignal depth(
        SQ0_16Signal signal,
        LinearRange<uint32_t> range
    ) {
        return [signal = std::move(signal), range](UQ0_16, TimeMillis elapsedMs) mutable -> uint32_t {
            return signal.sample(range, elapsedMs);
        };
    }

    DepthSignal depth(
        SQ0_16Signal signal,
        uint32_t scale,
        uint32_t offset
    ) {
        uint64_t max = static_cast<uint64_t>(offset) + static_cast<uint64_t>(scale);
        uint32_t maxDepth = (max > UINT32_MAX) ? UINT32_MAX : static_cast<uint32_t>(max);
        return depth(std::move(signal), LinearRange(offset, maxDepth));
    }
}
