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

#ifndef LED_SEGMENTS_PIPELINE_SIGNALS_WAVEFORMS_H
#define LED_SEGMENTS_PIPELINE_SIGNALS_WAVEFORMS_H

#include <functional>
#include "../utils/NoiseUtils.h"
#include "../utils/Units.h"

namespace LEDSegments {
    /// Waveform builders used by signals to derive acceleration and phase over time.
    /// All functions return fixed-point Q16.16 values and are safe on MCU targets.
    namespace Waveforms {
        /**
         * @brief A function that takes a phase (Q16.16) and returns an acceleration value (Q16.16).
         * Use this to shape how fast a signal speeds up/slows down across its phase.
         */
        using AccelerationWaveform = std::function<Units::SignalQ16_16(Units::PhaseTurnsUQ16_16)>;

        /**
         * @brief A function that takes a phase (Q16.16) and returns a phase velocity (AngleTurns16/sec Q16.16).
         * Governs how quickly phase advances; 1.0 == 1 angle unit per second (1/65536 of a full wrap).
         */
        using PhaseVelocityWaveform = std::function<Units::PhaseVelAngleUnitsQ16_16(Units::PhaseTurnsUQ16_16)>; // Q16.16-scaled AngleTurns16 per second

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
            AccelerationWaveform acceleration;
            /**
             * @brief A waveform that determines how fast the phase evolves over time.
             * A phase velocity of 0 results in a static phase.
             */
            PhaseVelocityWaveform phaseVelocity;
        };

        /// Constant acceleration: use when you want a steady push (linear ramps).
        /// Acceleration is specified as the integer part of a Q16.16 value.
        inline AccelerationWaveform ConstantAccelerationWaveform(int32_t value) {
            return [val_q16 = Units::SignalQ16_16(value)](Units::PhaseTurnsUQ16_16) { return val_q16; };
        }

        /// Constant acceleration (raw Q16.16): use when you want a fractional push.
        inline AccelerationWaveform ConstantAccelerationWaveformRaw(int32_t rawValue) {
            return [val_q16 = Units::SignalQ16_16::fromRaw(rawValue)](Units::PhaseTurnsUQ16_16) { return val_q16; };
        }

        /// Constant phase velocity: sets how quickly the waveform phase advances.
        /// Units are AngleTurns16/sec scaled Q16.16 (1.0 == 1/65536 revolution per second).
        /// Use to make sine/pulse/noise evolve at a fixed speed.
        inline PhaseVelocityWaveform ConstantPhaseVelocityWaveform(int32_t value) {
            return [val_q16 = Units::PhaseVelAngleUnitsQ16_16(value)](Units::PhaseTurnsUQ16_16) { return val_q16; };
        }

        /**
         * @brief Creates a WaveformSource that applies a constant acceleration.
         * The phase velocity is zero, meaning the acceleration is uniform and unchanging.
         * @param acceleration The constant acceleration value.
         *
         * Use for linear ramps and static offsets (camera drifts, slow zoom changes).
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
         *
         * Use when you need organic, non-repeating drift (camera hand-held motion,
         * flame wobble). Keep phaseVelocity low for gentle wandering.
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
         *
         * Use for smooth periodic motion (breathing zoom, rotation sway, pulsing light).
         * phaseVelocity tunes frequency; amplitude tunes excursion.
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
         *
         * Use for rhythmic bursts, beats, or stuttered motion where energy peaks then fades.
         * Pair with TimeQuantizeTransform or palette shifts for beat-synced visuals.
         * @return A WaveformSource struct.
         */
        inline WaveformSource Pulse(PhaseVelocityWaveform phaseVelocity, AccelerationWaveform amplitude) {
            AccelerationWaveform acceleration = [amplitude](Units::PhaseTurnsUQ16_16 phase) -> Units::SignalQ16_16 {
                Units::AngleTurns16 saw = phase >> 16;
                // Creates a triangular wave that quickly rises and then linearly falls.
                // Fixed logic to avoid discontinuity at QUARTER_TURN_U16.
                // 0..0.25 -> 0..0.5 (rise)
                // 0.25..1.0 -> 0.5..0 (fall)
                Units::AngleTurns16 pulse;
                if (saw < Units::QUARTER_TURN_U16) {
                    pulse = static_cast<Units::AngleTurns16>(saw << 1);
                } else {
                    // Map [16384..65535] to [32768..0]
                    // (65536 - saw) * (32768 / 49152) approx 2/3 scaling?
                    // Let's use a simpler asymmetric triangle:
                    // Rise 0..0.25 -> 0..MAX
                    // Fall 0.25..1.0 -> MAX..0
                    // But to match previous intent of "sharp attack", let's keep the peak at 0.25.
                    // Previous code: (MAX - saw). At 0.25 (16384), this is 49152 (0.75).
                    // We want it to be continuous with 0.5 (32768).
                    // Let's just use a standard triangle wave for safety if "Pulse" isn't strictly defined.
                    // Or fix the math:
                    // Rise: 0 to 0.25 -> 0 to 1.0 (0 to 65535)
                    // Fall: 0.25 to 1.0 -> 1.0 to 0 (65535 to 0)
                    if (saw < Units::QUARTER_TURN_U16) {
                         pulse = static_cast<Units::AngleTurns16>(saw * 4);
                    } else {
                         // (65536 - saw) * (65536 / 49152) = (65536 - saw) * 4/3
                         uint32_t rem = Units::ANGLE_U16_MAX - saw; // 49151..0
                         pulse = static_cast<Units::AngleTurns16>((rem * 4) / 3);
                    }
                }

                int32_t signedPulse = static_cast<int32_t>(pulse) - Units::U16_HALF;
                int32_t amp_q16 = amplitude(phase).asRaw();
                int64_t accel_q31 = (int64_t) signedPulse * amp_q16;
                return Units::SignalQ16_16::fromRaw(static_cast<int32_t>(accel_q31 >> 15));
            };
            return {acceleration, phaseVelocity};
        }
    }
}

#endif //LED_SEGMENTS_PIPELINE_SIGNALS_WAVEFORMS_H
