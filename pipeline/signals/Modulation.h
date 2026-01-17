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

#include <functional>
#include <memory>
#include <utility>
#include "polar/pipeline/utils/FixMathUtils.h"
#include "polar/pipeline/utils/MathUtils.h"
#include "polar/pipeline/utils/NoiseUtils.h"
#include "polar/pipeline/utils/Units.h"

namespace LEDSegments {
    template<typename T>
    using Modulation = std::function<T(TimeMillis)>;

    // Set to 0 to disable delta-time clamping for phase accumulation.
    inline constexpr TimeMillis MAX_DELTA_TIME_MS = 200;

    inline TimeMillis clampDeltaTime(TimeMillis deltaTime) {
        if (MAX_DELTA_TIME_MS == 0) return deltaTime;
        return (deltaTime > MAX_DELTA_TIME_MS) ? MAX_DELTA_TIME_MS : deltaTime;
    }

    template<typename T>
    inline Modulation<T> Constant(T value) {
        return [value](TimeMillis) { return value; };
    }

    inline FracQ16_16 clampSignalRaw(int64_t value) {
        if (value > INT32_MAX) value = INT32_MAX;
        if (value < INT32_MIN) value = INT32_MIN;
        return FracQ16_16::fromRaw(static_cast<int32_t>(value));
    }

    inline Modulation<FracQ16_16> ConstantSignal(FracQ16_16 value) {
        return Constant(value);
    }

    inline Modulation<FracQ16_16> ConstantSignalRaw(RawQ16_16 rawValue) {
        return Constant(FracQ16_16::fromRaw(raw(rawValue)));
    }

    inline Modulation<FracQ16_16> ConstantPhaseVelocity(FracQ16_16 value) {
        return Constant(value);
    }


    // PhaseAccumulator assumes modulo-2^32 wrap semantics and is only valid for angular/phase domains.
    class PhaseAccumulator {
    public:
        explicit PhaseAccumulator(Modulation<FracQ16_16> velocity)
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

    private:
        AngleTurnsUQ16_16 phase = AngleTurnsUQ16_16(0);
        TimeMillis lastTime = 0;
        bool hasLastTime = false;
        Modulation<FracQ16_16> phaseVelocity;
    };

    inline Modulation<FracQ16_16> Sine(
        Modulation<FracQ16_16> phaseVelocity,
        Modulation<FracQ16_16> amplitude,
        Modulation<FracQ16_16> offset
    ) {
        auto accumulator = std::make_shared<PhaseAccumulator>(std::move(phaseVelocity));
        return [accumulator,
                    amplitude = std::move(amplitude),
                    offset = std::move(offset)](TimeMillis time) {
            AngleTurnsUQ16_16 phase = accumulator->advance(time);
            TrigQ1_15 sin_val = sinQ1_15(phase);
            FracQ16_16 delta = mulTrigQ1_15_SignalQ16_16(sin_val, amplitude(time));
            int64_t sum_raw = delta.asRaw() + offset(time).asRaw();
            return clampSignalRaw(sum_raw);
        };
    }

    inline Modulation<FracQ16_16> Sine(
        Modulation<FracQ16_16> phaseVelocity,
        Modulation<FracQ16_16> amplitude
    ) {
        return Sine(std::move(phaseVelocity), std::move(amplitude), Constant(FracQ16_16(0)));
    }

    inline Modulation<FracQ16_16> Noise(
        Modulation<FracQ16_16> phaseVelocity,
        Modulation<FracQ16_16> amplitude,
        Modulation<FracQ16_16> offset
    ) {
        auto accumulator = std::make_shared<PhaseAccumulator>(std::move(phaseVelocity));
        return [accumulator,
                    amplitude = std::move(amplitude),
                    offset = std::move(offset)](TimeMillis time) {
            AngleTurnsUQ16_16 phase = accumulator->advance(time);
            NoiseRawU16 rawNoise = NoiseRawU16(inoise16(toFastLedPhase(phase)));
            NoiseNormU16 normNoise = normaliseNoise16(rawNoise);
            int16_t signedNoise = static_cast<int16_t>(
                static_cast<int32_t>(raw(normNoise)) - U16_HALF);
            TrigQ1_15 noise_val = TrigQ1_15(signedNoise);
            FracQ16_16 delta = mulTrigQ1_15_SignalQ16_16(noise_val, amplitude(time));
            int64_t sum_raw = delta.asRaw() + offset(time).asRaw();
            return clampSignalRaw(sum_raw);
        };
    }

    inline Modulation<FracQ16_16> Noise(
        Modulation<FracQ16_16> phaseVelocity,
        Modulation<FracQ16_16> amplitude
    ) {
        return Noise(std::move(phaseVelocity), std::move(amplitude), Constant(FracQ16_16(0)));
    }

    inline Modulation<FracQ16_16> Pulse(
        Modulation<FracQ16_16> phaseVelocity,
        Modulation<FracQ16_16> amplitude,
        Modulation<FracQ16_16> offset
    ) {
        auto accumulator = std::make_shared<PhaseAccumulator>(std::move(phaseVelocity));
        return [accumulator,
                    amplitude = std::move(amplitude),
                    offset = std::move(offset)](TimeMillis time) {
            AngleTurnsUQ16_16 phase = accumulator->advance(time);
            AngleUnitsQ0_16 saw = angleTurnsToAngleUnits(phase);
            uint16_t saw_raw = raw(saw);
            uint16_t pulse_raw = (saw_raw < HALF_TURN_U16)
                                     ? static_cast<uint16_t>(saw_raw << 1)
                                     : static_cast<uint16_t>((ANGLE_U16_MAX - saw_raw) << 1);
            int16_t signedPulse = static_cast<int16_t>(
                static_cast<int32_t>(pulse_raw) - U16_HALF);
            TrigQ1_15 pulse_val = TrigQ1_15(signedPulse);
            FracQ16_16 delta = mulTrigQ1_15_SignalQ16_16(pulse_val, amplitude(time));
            int64_t sum_raw = delta.asRaw() + offset(time).asRaw();
            return clampSignalRaw(sum_raw);
        };
    }

    inline Modulation<FracQ16_16> Pulse(
        Modulation<FracQ16_16> phaseVelocity,
        Modulation<FracQ16_16> amplitude
    ) {
        return Pulse(std::move(phaseVelocity), std::move(amplitude), Constant(FracQ16_16(0)));
    }
}

#endif //LED_SEGMENTS_PIPELINE_SIGNALS_FLUCTUATION_H
