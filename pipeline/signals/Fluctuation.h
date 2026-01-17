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

#include <climits>
#include <functional>
#include <memory>
#include <utility>
#include "polar/pipeline/utils/FixMathUtils.h"
#include "polar/pipeline/utils/MathUtils.h"
#include "polar/pipeline/utils/NoiseUtils.h"
#include "polar/pipeline/utils/Units.h"

namespace LEDSegments {
    template<typename T>
    using Fluctuation = std::function<T(Units::TimeMillis)>;

    namespace Fluctuations {
        // Set to 0 to disable delta-time clamping for phase accumulation.
        inline constexpr Units::TimeMillis MAX_DELTA_TIME_MS = 200;

        inline Units::TimeMillis clampDeltaTime(Units::TimeMillis deltaTime) {
            if (MAX_DELTA_TIME_MS == 0) return deltaTime;
            return (deltaTime > MAX_DELTA_TIME_MS) ? MAX_DELTA_TIME_MS : deltaTime;
        }

        template<typename T>
        inline Fluctuation<T> Constant(T value) {
            return [value](Units::TimeMillis) { return value; };
        }

        inline Fluctuation<Units::FracQ16_16> ConstantSignal(int32_t value) {
            return Constant(Units::FracQ16_16(value));
        }

        inline Fluctuation<Units::FracQ16_16> ConstantSignalRaw(int32_t rawValue) {
            return Constant(Units::FracQ16_16::fromRaw(rawValue));
        }

        inline Fluctuation<Units::FracQ16_16> ConstantPhaseVelocity(int32_t value) {
            return Constant(Units::FracQ16_16(value));
        }

        inline Units::FracQ16_16 clampSignalRaw(int64_t value) {
            if (value > INT32_MAX) value = INT32_MAX;
            if (value < INT32_MIN) value = INT32_MIN;
            return Units::FracQ16_16::fromRaw(static_cast<int32_t>(value));
        }

        // PhaseAccumulator assumes modulo-2^32 wrap semantics and is only valid for angular/phase domains.
        class PhaseAccumulator {
        public:
            explicit PhaseAccumulator(Fluctuation<Units::FracQ16_16> velocity)
                : phaseVelocity(std::move(velocity)) {}

            Units::AngleTurnsUQ16_16 advance(Units::TimeMillis time) {
                if (lastTime == 0) {
                    lastTime = time;
                    return phase;
                }
                Units::TimeMillis deltaTime = time - lastTime;
                lastTime = time;
                if (deltaTime == 0) return phase;

                deltaTime = clampDeltaTime(deltaTime);
                if (deltaTime == 0) return phase;

                Units::FracQ16_16 dt_q16 = millisToQ16_16(deltaTime);
                Units::RawFracQ16_16 phase_advance = mul_q16_16_wrap(phaseVelocity(time), dt_q16).asRaw();
                phase += static_cast<uint32_t>(phase_advance);
                return phase;
            }

        private:
            Units::AngleTurnsUQ16_16 phase = 0;
            Units::TimeMillis lastTime = 0;
            Fluctuation<Units::FracQ16_16> phaseVelocity;
        };

        inline Fluctuation<Units::FracQ16_16> Sine(
            Fluctuation<Units::FracQ16_16> phaseVelocity,
            Fluctuation<Units::FracQ16_16> amplitude,
            Fluctuation<Units::FracQ16_16> offset
        ) {
            auto accumulator = std::make_shared<PhaseAccumulator>(std::move(phaseVelocity));
            return [accumulator,
                    amplitude = std::move(amplitude),
                    offset = std::move(offset)](Units::TimeMillis time) {
                Units::AngleTurnsUQ16_16 phase = accumulator->advance(time);
                int32_t sin_val = sin16(static_cast<Units::AngleUnitsQ0_16>(phase >> 16));
                int32_t amp_raw = amplitude(time).asRaw();
                int64_t scaled = static_cast<int64_t>(sin_val) * amp_raw;
                int64_t delta_raw = scaled >> 15; // Q17.16
                int64_t sum_raw = delta_raw + offset(time).asRaw();
                return clampSignalRaw(sum_raw);
            };
        }

        inline Fluctuation<Units::FracQ16_16> Sine(
            Fluctuation<Units::FracQ16_16> phaseVelocity,
            Fluctuation<Units::FracQ16_16> amplitude
        ) {
            return Sine(std::move(phaseVelocity), std::move(amplitude), Constant(Units::FracQ16_16(0)));
        }

        inline Fluctuation<Units::FracQ16_16> Noise(
            Fluctuation<Units::FracQ16_16> phaseVelocity,
            Fluctuation<Units::FracQ16_16> amplitude,
            Fluctuation<Units::FracQ16_16> offset
        ) {
            auto accumulator = std::make_shared<PhaseAccumulator>(std::move(phaseVelocity));
            return [accumulator,
                    amplitude = std::move(amplitude),
                    offset = std::move(offset)](Units::TimeMillis time) {
                Units::AngleTurnsUQ16_16 phase = accumulator->advance(time);
                Units::NoiseRawU16 rawNoise = inoise16(static_cast<uint16_t>(phase >> 16));
                Units::NoiseNormU16 normNoise = normaliseNoise16(rawNoise);
                int32_t signedNoise = static_cast<int32_t>(normNoise) - Units::U16_HALF;
                int32_t amp_raw = amplitude(time).asRaw();
                int64_t scaled = static_cast<int64_t>(signedNoise) * amp_raw;
                int64_t delta_raw = scaled >> 15; // Q17.16
                int64_t sum_raw = delta_raw + offset(time).asRaw();
                return clampSignalRaw(sum_raw);
            };
        }

        inline Fluctuation<Units::FracQ16_16> Noise(
            Fluctuation<Units::FracQ16_16> phaseVelocity,
            Fluctuation<Units::FracQ16_16> amplitude
        ) {
            return Noise(std::move(phaseVelocity), std::move(amplitude), Constant(Units::FracQ16_16(0)));
        }

        inline Fluctuation<Units::FracQ16_16> Pulse(
            Fluctuation<Units::FracQ16_16> phaseVelocity,
            Fluctuation<Units::FracQ16_16> amplitude,
            Fluctuation<Units::FracQ16_16> offset
        ) {
            auto accumulator = std::make_shared<PhaseAccumulator>(std::move(phaseVelocity));
            return [accumulator,
                    amplitude = std::move(amplitude),
                    offset = std::move(offset)](Units::TimeMillis time) {
                Units::AngleTurnsUQ16_16 phase = accumulator->advance(time);
                Units::AngleUnitsQ0_16 saw = static_cast<Units::AngleUnitsQ0_16>(phase >> 16);
                Units::AngleUnitsQ0_16 pulse = (saw < Units::HALF_TURN_U16)
                                                ? static_cast<Units::AngleUnitsQ0_16>(saw << 1)
                                                : static_cast<Units::AngleUnitsQ0_16>((Units::ANGLE_U16_MAX - saw) << 1);
                int32_t signedPulse = static_cast<int32_t>(pulse) - Units::U16_HALF;
                int32_t amp_raw = amplitude(time).asRaw();
                int64_t scaled = static_cast<int64_t>(signedPulse) * amp_raw;
                int64_t delta_raw = scaled >> 15; // Q17.16
                int64_t sum_raw = delta_raw + offset(time).asRaw();
                return clampSignalRaw(sum_raw);
            };
        }

        inline Fluctuation<Units::FracQ16_16> Pulse(
            Fluctuation<Units::FracQ16_16> phaseVelocity,
            Fluctuation<Units::FracQ16_16> amplitude
        ) {
            return Pulse(std::move(phaseVelocity), std::move(amplitude), Constant(Units::FracQ16_16(0)));
        }
    }
}

#endif //LED_SEGMENTS_PIPELINE_SIGNALS_FLUCTUATION_H
