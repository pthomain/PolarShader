//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2023 Pierre Thomain

/*
 * This file is part of LED Segments.
 *
 * LED Segments is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LED Segments is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LED Segments. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LED_SEGMENTS_PIPELINE_SIGNALS_FLUCTUATION_H
#define LED_SEGMENTS_PIPELINE_SIGNALS_FLUCTUATION_H

#include <utility>
#include "polar/pipeline/utils/FixMathUtils.h"
#include "polar/pipeline/utils/MathUtils.h"
#include "polar/pipeline/utils/NoiseUtils.h"
#include "polar/pipeline/utils/TimeUtils.h"
#include "polar/pipeline/utils/Units.h"

namespace LEDSegments {
    using ScalarModulation = fl::function<FracQ16_16(TimeMillis)>;

    namespace detail {

        // PhaseAccumulator assumes modulo-2^32 wrap semantics and is only valid for angular/phase domains.
        struct PhaseAccumulator {
            AngleTurnsUQ16_16 phase{0};
            TimeMillis lastTime{0};
            bool hasLastTime{false};
            // phaseVelocity returns signed turns-per-second in Q16.16.
            ScalarModulation phaseVelocity;

            explicit PhaseAccumulator(ScalarModulation velocity)
                : phaseVelocity(std::move(velocity)) {
            }

            AngleTurnsUQ16_16 advance(TimeMillis time) {
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

                FracQ16_16 dt_q16 = millisToQ16_16(deltaTime);
                RawQ16_16 phase_advance = RawQ16_16(mul_q16_16_wrap(phaseVelocity(time), dt_q16).asRaw());
                phase = wrapAddSigned(phase, raw(phase_advance));
                return phase;
            }
        };

        template <typename SampleFn>
        inline ScalarModulation makeWave(
            ScalarModulation phaseVelocity,
            ScalarModulation amplitude,
            ScalarModulation offset,
            SampleFn sample
        ) {
            PhaseAccumulator acc{std::move(phaseVelocity)};

            return [acc = std::move(acc),
                    amplitude = std::move(amplitude),
                    offset = std::move(offset),
                    sample = std::move(sample)](TimeMillis time) mutable -> FracQ16_16 {

                AngleTurnsUQ16_16 phase = acc.advance(time);
                TrigQ1_15 v = sample(phase);

                FracQ16_16 delta = mulTrigQ1_15_SignalQ16_16(v, amplitude(time));
                int64_t sum_raw = (int64_t)delta.asRaw() + offset(time).asRaw();
                return clamp_q16_16_raw(sum_raw);
            };
        }
    }

    inline ScalarModulation Constant(FracQ16_16 value) {
        return [value](TimeMillis) { return value; };
    }

    inline ScalarModulation ConstantRawQ16_16(RawQ16_16 rawValue) {
        return Constant(FracQ16_16::fromRaw(raw(rawValue)));
    }

    inline ScalarModulation ConstantPhaseVelocity(FracQ16_16 value) {
        return Constant(value);
    }

    inline ScalarModulation Sine(
        ScalarModulation phaseVelocity,
        ScalarModulation amplitude,
        ScalarModulation offset
    ) {
        return detail::makeWave(std::move(phaseVelocity), std::move(amplitude), std::move(offset),
                                [](AngleTurnsUQ16_16 phase) -> TrigQ1_15 {
                                    return sinQ1_15(phase);
                                }
        );
    }

    inline ScalarModulation Sine(
        ScalarModulation phaseVelocity,
        ScalarModulation amplitude
    ) {
        return Sine(std::move(phaseVelocity), std::move(amplitude), Constant(FracQ16_16(0)));
    }

    inline ScalarModulation Noise(
        ScalarModulation phaseVelocity,
        ScalarModulation amplitude,
        ScalarModulation offset
    ) {
        return detail::makeWave(std::move(phaseVelocity), std::move(amplitude), std::move(offset),
                                [](AngleTurnsUQ16_16 phase) -> TrigQ1_15 {
                                    NoiseRawU16 rawNoise = NoiseRawU16(inoise16(toFastLedPhase(phase)));
                                    NoiseNormU16 normNoise = normaliseNoise16(rawNoise);
                                    int16_t signedNoise = static_cast<int16_t>(
                                        static_cast<int32_t>(raw(normNoise)) - U16_HALF);
                                    return TrigQ1_15(signedNoise);
                                }
        );
    }

    inline ScalarModulation Noise(
        ScalarModulation phaseVelocity,
        ScalarModulation amplitude
    ) {
        return Noise(std::move(phaseVelocity), std::move(amplitude), Constant(FracQ16_16(0)));
    }

    inline ScalarModulation Pulse(
        ScalarModulation phaseVelocity,
        ScalarModulation amplitude,
        ScalarModulation offset
    ) {
        return detail::makeWave(std::move(phaseVelocity), std::move(amplitude), std::move(offset),
                                [](AngleTurnsUQ16_16 phase) -> TrigQ1_15 {
                                    AngleUnitsQ0_16 saw = angleTurnsToAngleUnits(phase);
                                    uint16_t saw_raw = raw(saw);
                                    uint16_t pulse_raw = (saw_raw < HALF_TURN_U16)
                                                             ? static_cast<uint16_t>(saw_raw << 1)
                                                             : static_cast<uint16_t>((ANGLE_U16_MAX - saw_raw) << 1);
                                    int16_t signedPulse = static_cast<int16_t>(
                                        static_cast<int32_t>(pulse_raw) - U16_HALF);
                                    return TrigQ1_15(signedPulse);
                                }
        );
    }

    inline ScalarModulation Pulse(
        ScalarModulation phaseVelocity,
        ScalarModulation amplitude
    ) {
        return Pulse(std::move(phaseVelocity), std::move(amplitude), Constant(FracQ16_16(0)));
    }
}

#endif //LED_SEGMENTS_PIPELINE_SIGNALS_FLUCTUATION_H
