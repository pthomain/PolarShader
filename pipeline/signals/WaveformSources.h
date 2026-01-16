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

#ifndef LED_SEGMENTS_PIPELINE_SIGNALS_WAVEFORMSOURCES_H
#define LED_SEGMENTS_PIPELINE_SIGNALS_WAVEFORMSOURCES_H

#include "../utils/NoiseUtils.h"
#include "../utils/Units.h"
#include "Waveforms.h"

namespace LEDSegments {
    namespace WaveformSources {
        /**
         * @brief Defines the behavior of a Signal.
         *
         * A WaveformSource provides the core drivers for a Signal's movement. It consists
         * of two separate waveforms: one for acceleration and one for the rate of change
         * of the phase itself. Compose these to create oscillations, pulses, noise-driven
         * motion, or static drifts.
         */
        struct WaveformSource {
            /**
             * @brief A waveform that determines the acceleration of a Signal at a given phase.
             * Think “shape of the movement” — e.g. sine, pulse, noise.
             */
            Waveforms::AccelerationWaveform acceleration;
            /**
             * @brief A waveform that determines how fast the phase evolves over time.
             * A phase velocity of 0 results in a static phase.
             */
            Waveforms::PhaseVelocityWaveform phaseVelocity;
        };

        /**
         * @brief Creates a WaveformSource that applies a constant acceleration.
         * The phase velocity is zero, meaning the acceleration is uniform and unchanging.
         * @param acceleration The constant acceleration value.
         *
         * Use for linear ramps and static offsets (camera drifts, slow zoom changes).
         * @return A WaveformSource struct.
         */
        inline WaveformSource Constant(int32_t acceleration) {
            return {
                Waveforms::ConstantAccelerationWaveform(acceleration),
                Waveforms::ConstantPhaseVelocityWaveform(0)
            };
        }

        /**
         * @brief Creates a WaveformSource driven by Perlin noise.
         * The acceleration of the signal will follow a smooth, random pattern.
         * @param phaseVelocity A waveform controlling how quickly the noise pattern is traversed.
         * @param amplitude A waveform controlling the strength (amplitude) of the noise.
         *
         * Use when you need organic, non-repeating drift (camera hand-held motion,
         * flame wobble). Keep phaseVelocity low for gentle wandering.
         * @return A WaveformSource struct.
         */
        inline WaveformSource Noise(
            Waveforms::PhaseVelocityWaveform phaseVelocity,
            Waveforms::AccelerationWaveform amplitude
        ) {
            Waveforms::AccelerationWaveform acceleration = [amplitude](Units::PhaseTurnsUQ16_16 phase)
                -> Units::SignalQ16_16 {
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
         *
         * Use for smooth periodic motion (breathing zoom, rotation sway, pulsing light).
         * phaseVelocity tunes frequency; amplitude tunes excursion.
         * @return A WaveformSource struct.
         */
        inline WaveformSource Sine(
            Waveforms::PhaseVelocityWaveform phaseVelocity,
            Waveforms::AccelerationWaveform amplitude
        ) {
            Waveforms::AccelerationWaveform acceleration = [amplitude](Units::PhaseTurnsUQ16_16 phase)
                -> Units::SignalQ16_16 {
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
         *
         * Use for rhythmic bursts, beats, or stuttered motion where energy peaks then fades.
         * Pair with TimeQuantizeTransform or palette shifts for beat-synced visuals.
         * @return A WaveformSource struct.
         */
        inline WaveformSource Pulse(
            Waveforms::PhaseVelocityWaveform phaseVelocity,
            Waveforms::AccelerationWaveform amplitude
        ) {
            Waveforms::AccelerationWaveform acceleration = [amplitude](Units::PhaseTurnsUQ16_16 phase)
                -> Units::SignalQ16_16 {
                Units::AngleTurns16 saw = phase >> 16;
                // Standard triangle: 0..0.5 rise to max, 0.5..1 fall to 0.
                Units::AngleTurns16 pulse = (saw < Units::HALF_TURN_U16)
                                                ? static_cast<Units::AngleTurns16>(saw << 1)
                                                : static_cast<Units::AngleTurns16>((Units::ANGLE_U16_MAX - saw) << 1);

                int32_t signedPulse = static_cast<int32_t>(pulse) - Units::U16_HALF;
                int32_t amp_q16 = amplitude(phase).asRaw();
                int64_t accel_q31 = (int64_t) signedPulse * amp_q16;
                return Units::SignalQ16_16::fromRaw(static_cast<int32_t>(accel_q31 >> 15));
            };
            return {acceleration, phaseVelocity};
        }
    }
}

#endif //LED_SEGMENTS_PIPELINE_SIGNALS_WAVEFORMSOURCES_H
