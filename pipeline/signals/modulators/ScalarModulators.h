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

#ifndef LED_SEGMENTS_PIPELINE_SIGNALS_MODULATORS_SCALAR_H
#define LED_SEGMENTS_PIPELINE_SIGNALS_MODULATORS_SCALAR_H

#include <utility>

#include "polar/pipeline/utils/FixMathUtils.h"
#include "polar/pipeline/utils/MathUtils.h"
#include "polar/pipeline/utils/NoiseUtils.h"
#include "polar/pipeline/utils/TimeUtils.h"
#include "polar/pipeline/utils/Units.h"

namespace LEDSegments {
    /**
     * @brief Time-indexed scalar signal used for animation and modulation.
     *
     * Use when:
     * - You need a value that changes over time (e.g., speed, amplitude, offsets).
     * - The consumer expects Q16.16 fixed-point semantics.
     *
     * Avoid when:
     * - You need angular phase; use AngleModulator instead.
     * - You need per-frame stateful integration that isn't a function of time.
     */
    using ScalarModulator = fl::function<FracQ16_16(TimeMillis)>;

    namespace detail {
        // PhaseAccumulator assumes modulo-2^32 wrap semantics and is only valid for angular/phase domains.
        struct PhaseAccumulator {
            AngleTurnsUQ16_16 phase{0};
            TimeMillis lastTime{0};
            bool hasLastTime{false};
            // phaseVelocity returns signed turns-per-second in Q16.16.
            ScalarModulator phaseVelocity;

            explicit PhaseAccumulator(ScalarModulator velocity)
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

        template<typename SampleFn>
        inline ScalarModulator makeWave(
            ScalarModulator phaseVelocity,
            ScalarModulator amplitude,
            ScalarModulator offset,
            SampleFn sample
        ) {
            PhaseAccumulator acc{std::move(phaseVelocity)};

            return [acc = std::move(acc),
                        amplitude = std::move(amplitude),
                        offset = std::move(offset),
                        sample = std::move(sample)
                    ](TimeMillis time) mutable -> FracQ16_16 {
                AngleTurnsUQ16_16 phase = acc.advance(time);
                TrigQ1_15 v = sample(phase);

                FracQ16_16 delta = mulTrigQ1_15_SignalQ16_16(v, amplitude(time));
                int64_t sum_raw = delta.asRaw() + offset(time).asRaw();
                return clamp_q16_16_raw(sum_raw);
            };
        }
    }

    /**
     * @brief Constant scalar signal (Q16.16).
     *
     * Use when:
     * - You need a fixed value (e.g., constant speed or amplitude).
     *
     * Avoid when:
     * - You need time-varying behavior; use a wave or custom modulator.
     */
    inline ScalarModulator Constant(FracQ16_16 value) {
        return [value](TimeMillis) { return value; };
    }

    /**
     * @brief Constant scalar signal from raw Q16.16 storage.
     *
     * Use when:
     * - You already have a raw Q16.16 value and want a constant modulator.
     *
     * Avoid when:
     * - You are not sure of the raw encoding; prefer Constant(FracQ16_16).
     */
    inline ScalarModulator ConstantRawQ16_16(RawQ16_16 rawValue) {
        return Constant(FracQ16_16::fromRaw(raw(rawValue)));
    }

    /**
     * @brief Alias for a constant phase velocity in turns/sec (Q16.16).
     *
     * Use when:
     * - You are feeding a phase accumulator or a wave generator.
     *
     * Avoid when:
     * - You need to clamp or integrate; use IntegrateAngle on the angular side.
     */
    inline ScalarModulator ConstantPhaseVelocity(FracQ16_16 value) {
        return Constant(value);
    }

    /**
     * @brief Sine wave generator (phase-driven).
     *
     * Parameters:
     * - phaseVelocity: turns/sec in Q16.16.
     * - amplitude/offset: Q16.16 output shaping.
     *
     * Use when:
     * - You need smooth periodic modulation of a scalar signal.
     *
     * Avoid when:
     * - You need an angular phase output; use IntegrateAngle and sample separately.
     */
    inline ScalarModulator Sine(
        ScalarModulator phaseVelocity,
        ScalarModulator amplitude,
        ScalarModulator offset
    ) {
        return detail::makeWave(
            std::move(phaseVelocity),
            std::move(amplitude),
            std::move(offset),
            [](AngleTurnsUQ16_16 phase) -> TrigQ1_15 {
                return sinQ1_15(phase);
            }
        );
    }

    inline ScalarModulator Sine(
        ScalarModulator phaseVelocity,
        ScalarModulator amplitude
    ) {
        return Sine(
            std::move(phaseVelocity),
            std::move(amplitude),
            Constant(FracQ16_16(0))
        );
    }

    /**
     * @brief Noise wave generator (phase-driven).
     *
     * Use when:
     * - You want deterministic pseudo-random modulation based on time.
     *
     * Avoid when:
     * - You need reproducibility across different noise implementations; keep tests host-only.
     */
    inline ScalarModulator Noise(
        ScalarModulator phaseVelocity,
        ScalarModulator amplitude,
        ScalarModulator offset
    ) {
        return detail::makeWave(
            std::move(phaseVelocity),
            std::move(amplitude),
            std::move(offset),
            [](AngleTurnsUQ16_16 phase) -> TrigQ1_15 {
                NoiseRawU16 rawNoise = NoiseRawU16(inoise16(toFastLedPhase(phase)));
                NoiseNormU16 normNoise = normaliseNoise16(rawNoise);
                int16_t signedNoise = static_cast<int16_t>(
                    static_cast<int32_t>(raw(normNoise)) - U16_HALF);
                return TrigQ1_15(signedNoise);
            }
        );
    }

    inline ScalarModulator Noise(
        ScalarModulator phaseVelocity,
        ScalarModulator amplitude
    ) {
        return Noise(
            std::move(phaseVelocity),
            std::move(amplitude),
            Constant(FracQ16_16(0))
        );
    }

    /**
     * @brief Pulse/triangle wave generator (phase-driven).
     *
     * Use when:
     * - You need a sharper periodic modulation than sine.
     *
     * Avoid when:
     * - You need a band-limited signal; this is a simple waveform.
     */
    inline ScalarModulator Pulse(
        ScalarModulator phaseVelocity,
        ScalarModulator amplitude,
        ScalarModulator offset
    ) {
        return detail::makeWave(
            std::move(phaseVelocity),
            std::move(amplitude),
            std::move(offset),
            [](AngleTurnsUQ16_16 phase) -> TrigQ1_15 {
                AngleUnitsQ0_16 saw = angleTurnsToAngleUnits(phase);
                uint16_t saw_raw = raw(saw);
                uint16_t pulse_raw = (saw_raw < HALF_TURN_U16)
                                         ? static_cast<uint16_t>(saw_raw << 1)
                                         : static_cast<uint16_t>((ANGLE_U16_MAX - saw_raw) << 1);
                int16_t signedPulse = static_cast<int16_t>(static_cast<int32_t>(pulse_raw) - U16_HALF);
                return TrigQ1_15(signedPulse);
            }
        );
    }

    inline ScalarModulator Pulse(
        ScalarModulator phaseVelocity,
        ScalarModulator amplitude
    ) {
        return Pulse(
            std::move(phaseVelocity),
            std::move(amplitude),
            Constant(FracQ16_16(0))
        );
    }
}

#endif //LED_SEGMENTS_PIPELINE_SIGNALS_MODULATORS_SCALAR_H
