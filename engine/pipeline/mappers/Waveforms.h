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
         * @brief A function that takes a phase (Q16.16) and returns a value (Q16.16).
         * The phase represents a point in time or space along the waveform.
         */
        using Waveform = std::function<int32_t(uint32_t)>; // Q16.16 -> Q16.16

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
            Waveform acceleration;
            /**
             * @brief A waveform that determines how fast the phase evolves over time.
             * A phase velocity of 0 results in a static phase.
             */
            Waveform phaseVelocity;
        };

        /**
         * @brief Creates a waveform that returns a constant value, regardless of the phase.
         * @param value The constant value (integer part) for the waveform to return.
         * @return A Waveform function.
         */
        inline Waveform ConstantWaveform(int32_t value) {
            return [val_q16 = value << 16](uint32_t) { return val_q16; };
        }

        /**
         * @brief Creates a WaveformSource that applies a constant acceleration.
         * The phase velocity is zero, meaning the acceleration is uniform and unchanging.
         * @param acceleration The constant acceleration value.
         * @return A WaveformSource struct.
         */
        inline WaveformSource Constant(int32_t acceleration) {
            return {ConstantWaveform(acceleration), ConstantWaveform(0)};
        }

        /**
         * @brief Creates a WaveformSource driven by Perlin noise.
         * The acceleration of the signal will follow a smooth, random pattern.
         * @param phaseVelocity A waveform controlling how quickly the noise pattern is traversed.
         * @param amplitude A waveform controlling the strength (amplitude) of the noise.
         * @return A WaveformSource struct.
         */
        inline WaveformSource Noise(Waveform phaseVelocity, Waveform amplitude) {
            Waveform acceleration = [amplitude](uint32_t phase) -> int32_t {
                uint16_t rawNoise = inoise16(phase >> 16);
                uint16_t normNoise = normaliseNoise16(rawNoise);
                int32_t signedNoise = (int32_t) normNoise - 32768;
                int32_t amp_q16 = amplitude(phase);
                int64_t accel_q31 = (int64_t) signedNoise * amp_q16;
                return (int32_t) (accel_q31 >> 15);
            };
            return {acceleration, phaseVelocity};
        }

        /**
         * @brief Creates a WaveformSource that oscillates using a sine wave.
         * @param phaseVelocity A waveform controlling the frequency of the sine wave.
         * @param amplitude A waveform controlling the amplitude of the sine wave.
         * @return A WaveformSource struct.
         */
        inline WaveformSource Sine(Waveform phaseVelocity, Waveform amplitude) {
            Waveform acceleration = [amplitude](uint32_t phase) -> int32_t {
                int32_t sin_val = sin16(phase >> 16);
                int32_t amp_q16 = amplitude(phase);
                int64_t accel_q31 = (int64_t) sin_val * amp_q16;
                return (int32_t) (accel_q31 >> 15);
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
        inline WaveformSource Pulse(Waveform phaseVelocity, Waveform amplitude) {
            Waveform acceleration = [amplitude](uint32_t phase) -> int32_t {
                uint16_t saw = phase >> 16;
                // Creates a triangular wave that quickly rises and then linearly falls.
                uint16_t pulse = (saw < 16384) ? (saw << 1) : (65535 - saw);
                int32_t signedPulse = (int32_t) pulse - 32768;
                int32_t amp_q16 = amplitude(phase);
                int64_t accel_q31 = (int64_t) signedPulse * amp_q16;
                return (int32_t) (accel_q31 >> 15);
            };
            return {acceleration, phaseVelocity};
        }
    }
}

#endif //LED_SEGMENTS_EFFECTS_MAPPERS_WAVEFORMS_H
