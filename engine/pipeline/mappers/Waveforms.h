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

#ifndef LED_SEGMENTS_EFFECTS_MAPPERS_WAVEFORMS_H
#define LED_SEGMENTS_EFFECTS_MAPPERS_WAVEFORMS_H

#include <functional>
#include "../utils/MathUtils.h"
#include "../utils/NoiseUtils.h"
#include "../utils/FixMathUtils.h"
#include "../utils/Units.h"

namespace LEDSegments {
    /**
     * @brief A collection of functions to generate waveforms for driving Signals.
     *
     * This namespace provides factory functions to create various 'WaveformSource' objects.
     * A WaveformSource defines the behavior of a Signal by providing its acceleration
     * and phase velocity over time. This allows for creating complex, dynamic movements
     * like oscillations, noise-based randomness, and pulses.
     *
     * All calculations are performed using Q16.16 fixed-point arithmetic.
     */
    namespace Waveforms {
        /**
         * @brief A function that takes a phase (Q16.16) and returns an acceleration value (Q16.16).
         */
        using AccelerationWaveform = std::function<Units::SignalQ16_16(Units::PhaseTurnsUQ16_16)>;

        /**
         * @brief A function that takes a phase (Q16.16) and returns a phase velocity (Turns/s Q16.16).
         */
        using PhaseVelocityWaveform = std::function<Units::PhaseVelAngleUnitsQ16_16(Units::PhaseTurnsUQ16_16)>; // Q16.16-scaled AngleTurns16 per second

        /**
         * @brief Defines the behavior of a Signal.
         *
         * A WaveformSource provides the core drivers for a Signal's movement. It consists
         * of two separate waveforms: one for acceleration and one for the rate of change
         * of the phase itself.
         */
        struct WaveformSource {
            /**
             * @brief A waveform that determines the acceleration of a Signal at a given phase.
             */
            AccelerationWaveform acceleration;
            /**
             * @brief A waveform that determines how fast the phase evolves over time.
             * A phase velocity of 0 results in a static phase.
             */
            PhaseVelocityWaveform phaseVelocity;
        };

        /**
         * @brief Creates a waveform that returns a constant acceleration value.
         * @param value The constant acceleration value (integer part).
         * @return An AccelerationWaveform function.
         */
        inline AccelerationWaveform ConstantAccelerationWaveform(int32_t value) {
            return [val_q16 = Units::SignalQ16_16(value)](Units::PhaseTurnsUQ16_16) { return val_q16; };
        }

        /**
         * @brief Creates a waveform that returns a constant phase velocity value.
         * @param value The constant phase velocity value (integer part, in turns/sec).
         * @return A PhaseVelocityWaveform function.
         */
        inline PhaseVelocityWaveform ConstantPhaseVelocityWaveform(int32_t value) {
            return [val_q16 = Units::PhaseVelAngleUnitsQ16_16(value)](Units::PhaseTurnsUQ16_16) { return val_q16; };
        }

        /**
         * @brief Creates a WaveformSource that applies a constant acceleration.
         * The phase velocity is zero, meaning the acceleration is uniform and unchanging.
         * @param acceleration The constant acceleration value.
         * @return A WaveformSource struct.
         */
        inline WaveformSource Constant(int32_t acceleration) {
            return {ConstantAccelerationWaveform(acceleration), ConstantPhaseVelocityWaveform(0)};
        }

        /**
         * @brief Creates a WaveformSource driven by Perlin noise.
         * The acceleration of the signal will follow a smooth, random pattern.
         * @param phaseVelocity A waveform controlling how quickly the noise pattern is traversed.
         * @param amplitude A waveform controlling the strength (amplitude) of the noise.
         * @return A WaveformSource struct.
         */
        inline WaveformSource Noise(PhaseVelocityWaveform phaseVelocity, AccelerationWaveform amplitude) {
            AccelerationWaveform acceleration = [amplitude](Units::PhaseTurnsUQ16_16 phase) -> Units::SignalQ16_16 {
                Units::NoiseRawU16 rawNoise = inoise16(phase >> 16);
                Units::NoiseNormU16 normNoise = normaliseNoise16(rawNoise);
                int32_t signedNoise = static_cast<int32_t>(normNoise) - Units::U16_HALF;
                int32_t amp_q16 = amplitude(phase).asRaw();
                int64_t accel_q31 = (int64_t) signedNoise * amp_q16;
                return Units::SignalQ16_16::fromRaw(static_cast<int32_t>(accel_q31 >> 15));
            };
            return {acceleration, phaseVelocity};
        }

        /**
         * @brief Creates a WaveformSource that oscillates using a sine wave.
         * @param phaseVelocity A waveform controlling the frequency of the sine wave.
         * @param amplitude A waveform controlling the amplitude of the sine wave.
         * @return A WaveformSource struct.
         */
        inline WaveformSource Sine(PhaseVelocityWaveform phaseVelocity, AccelerationWaveform amplitude) {
            AccelerationWaveform acceleration = [amplitude](Units::PhaseTurnsUQ16_16 phase) -> Units::SignalQ16_16 {
                int32_t sin_val = sin16(phase >> 16);
                int32_t amp_q16 = amplitude(phase).asRaw();
                int64_t accel_q31 = (int64_t) sin_val * amp_q16;
                return Units::SignalQ16_16::fromRaw(static_cast<int32_t>(accel_q31 >> 15));
            };
            return {acceleration, phaseVelocity};
        }

        /**
         * @brief Creates a WaveformSource with a sharp attack and linear decay.
         * This creates a "pulse" or "heartbeat" effect.
         * @param phaseVelocity A waveform controlling the frequency of the pulses.
         * @param amplitude A waveform controlling the amplitude of the pulses.
         * @return A WaveformSource struct.
         */
        inline WaveformSource Pulse(PhaseVelocityWaveform phaseVelocity, AccelerationWaveform amplitude) {
            AccelerationWaveform acceleration = [amplitude](Units::PhaseTurnsUQ16_16 phase) -> Units::SignalQ16_16 {
                Units::AngleTurns16 saw = phase >> 16;
                // Creates a triangular wave that quickly rises and then linearly falls.
                Units::AngleTurns16 pulse = (saw < Units::QUARTER_TURN_U16) ? static_cast<Units::AngleTurns16>(saw << 1)
                                                                            : static_cast<Units::AngleTurns16>(Units::ANGLE_U16_MAX - saw);
                int32_t signedPulse = static_cast<int32_t>(pulse) - Units::U16_HALF;
                int32_t amp_q16 = amplitude(phase).asRaw();
                int64_t accel_q31 = (int64_t) signedPulse * amp_q16;
                return Units::SignalQ16_16::fromRaw(static_cast<int32_t>(accel_q31 >> 15));
            };
            return {acceleration, phaseVelocity};
        }
    }
}

#endif //LED_SEGMENTS_EFFECTS_MAPPERS_WAVEFORMS_H
