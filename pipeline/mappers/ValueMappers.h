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

#ifndef LED_SEGMENTS_EFFECTS_MAPPERS_VALUEMAPPERS_H
#define LED_SEGMENTS_EFFECTS_MAPPERS_VALUEMAPPERS_H

#include <functional>
#include "../utils/MathUtils.h"
#include "../utils/NoiseUtils.h"

namespace LEDSegments {
    static constexpr uint16_t MILLIS_PER_SECOND = 1000;

    namespace Waveforms {
        /**
         * @brief A stateless function that generates a value from a given phase.
         * @param phase A Q16.16 fixed-point phase value.
         * @return A Q16.16 fixed-point value.
         */
        using Waveform = std::function<int32_t(uint32_t)>;

        /**
         * @brief A pair of waveforms describing a dynamic value: one for the acceleration
         * and one for the rate of change of its phase (phase velocity).
         */
        struct WaveformSource {
            Waveform acceleration;
            Waveform phaseVelocity;
        };

        /**
         * @brief Creates a waveform that returns a constant value, regardless of phase.
         * @param value The integer value, which will be converted to Q16.16 format.
         */
        inline Waveform ConstantWaveform(int32_t value) {
            return [val_q16 = value << 16](uint32_t) { return val_q16; };
        }

        /**
         * @brief Creates a WaveformSource that produces a constant acceleration and has zero phase velocity.
         * @param acceleration The constant acceleration value in integer domain units.
         */
        inline WaveformSource Constant(int32_t acceleration) {
            return {ConstantWaveform(acceleration), ConstantWaveform(0)};
        }

        /**
         * @brief Creates a WaveformSource that generates a signed Perlin noise acceleration.
         * @param phaseVelocity A waveform providing the rate of phase advancement in "noise units per second" (Q16.16).
         * @param amplitude A waveform providing the peak acceleration in integer units/sec^2 (Q16.16).
         * @return A WaveformSource containing the noise-based acceleration and the provided phase velocity.
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
         * @brief Creates a WaveformSource that generates a sine wave acceleration.
         * @param phaseVelocity A waveform providing the rate of phase advancement in "turns per second" (Q16.16).
         * @param amplitude A waveform providing the peak acceleration in integer units/sec^2 (Q16.16).
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
         * @brief Creates a WaveformSource that generates a pulse wave acceleration.
         * The pulse has a sharp attack and a slower, linear decay.
         * @param phaseVelocity A waveform providing the rate of phase advancement in "turns per second" (Q16.16).
         * @param amplitude A waveform providing the peak acceleration in integer units/sec^2 (Q16.16).
         */
        inline WaveformSource Pulse(Waveform phaseVelocity, Waveform amplitude) {
            Waveform acceleration = [amplitude](uint32_t phase) -> int32_t {
                uint16_t saw = phase >> 16;
                // Create a pulse with a sharp attack and slower decay
                uint16_t pulse = (saw < 16384) ? (saw << 1) : (65535 - saw);
                int32_t signedPulse = (int32_t) pulse - 32768;

                int32_t amp_q16 = amplitude(phase);
                int64_t accel_q31 = (int64_t) signedPulse * amp_q16;
                return (int32_t) (accel_q31 >> 15);
            };
            return {acceleration, phaseVelocity};
        }
    }

    // --- Signal Policies ---

    struct LinearPolicy {
        void apply(int32_t &, int32_t &) const {
        }
    };

    struct ClampPolicy {
        using Q16_16 = int32_t;
        Q16_16 min_val;
        Q16_16 max_val;

        /**
         * @param min The minimum bound in integer domain units.
         * @param max The maximum bound in integer domain units.
         */
        ClampPolicy(int32_t min, int32_t max) : min_val(min << 16), max_val(max << 16) {
        }

        void apply(Q16_16 &position, Q16_16 &velocity) const {
            if (position < min_val) {
                position = min_val;
                velocity = 0;
            } else if (position > max_val) {
                position = max_val;
                velocity = 0;
            }
        }
    };

    /**
     * @brief A policy that wraps a signal's position, useful for angular values.
     * The default wrap value of 65536 makes it suitable for angles measured in "turns",
     * where 1.0 turn = 65536 units, matching the uint16_t range.
     */
    struct WrapPolicy {
        using Q16_16 = int32_t;
        Q16_16 wrap_val;

        WrapPolicy(int32_t wrap = 65536) : wrap_val(wrap << 16) {
        }

        void apply(Q16_16 &position, Q16_16 &) const {
            // This robust version handles multiple wraps in a single time step.
            if (position >= wrap_val || position < 0) {
                position %= wrap_val;
                if (position < 0) {
                    position += wrap_val;
                }
            }
        }
    };

    /**
     * @class Signal
     * @brief A stateful, 1D physical model that owns and integrates time and phase.
     * @property retention_per_sec The fraction of velocity retained after one second of damping.
     *           This is a dimensionless value in the range [0.0, 1.0).
     */
    template<typename Policy>
    class Signal {
    public:
        using Time = unsigned long;
        using Value = int32_t;
        using Q16_16 = int32_t;

    private:
        Q16_16 position;
        Q16_16 velocity = 0;
        Waveforms::WaveformSource source;
        uint32_t phase_q16 = 0;
        fract16 retention_per_sec;
        Time lastTime = 0;
        Policy policy;

    public:
        /**
         * @param initialPosition The starting position of the signal.
         * @param waveformSource The source of acceleration and phase velocity for the signal's physics.
         * @param retention_per_sec_0_1000 An integer from 0-999 representing the percentage of
         *        velocity to retain per second, scaled by 10. E.g., 900 means 90% retention.
         *        A value of 1000 (100%) is disallowed to ensure all motion eventually stops.
         */
        template<typename... PolicyArgs>
        Signal(
            Value initialPosition = 0,
            Waveforms::WaveformSource waveformSource = Waveforms::Constant(0),
            uint16_t retention_per_sec_0_1000 = 900,
            PolicyArgs... policy_args
        ) : position(initialPosition << 16),
            source(std::move(waveformSource)),
            policy(policy_args...) {
            // Constrain input to prevent retention >= 1.0, which would be unstable.
            uint16_t constrained_retention = min((uint16_t) 999, retention_per_sec_0_1000);
            retention_per_sec = divide_u16_as_fract16(constrained_retention, 1000);
            phase_q16 = (uint32_t) random16() << 16;
        }

        void advanceFrame(Time currentTime) {
            if (lastTime == 0) {
                lastTime = currentTime;
                return;
            }

            Time deltaTime = currentTime - lastTime;
            lastTime = currentTime;
            if (deltaTime == 0) return;

            // Clamp deltaTime to a max of 200ms to prevent instability or large jumps
            // after a long pause. This ensures smooth resumption of motion.
            const Time MAX_DELTA_TIME = 200;
            if (deltaTime > MAX_DELTA_TIME) {
                deltaTime = MAX_DELTA_TIME;
            }

            const Time CHUNK_SIZE_MS = 100;
            Time remaining_dt = deltaTime;

            while (remaining_dt > 0) {
                Time chunk = min(remaining_dt, CHUNK_SIZE_MS);
                remaining_dt -= chunk;

                // 1. Get phase velocity and advance phase
                int32_t phase_velocity_q16 = source.phaseVelocity(phase_q16);
                int64_t advance_q16 = ((int64_t) phase_velocity_q16 * chunk) / MILLIS_PER_SECOND;
                phase_q16 += (uint32_t) advance_q16;

                // 2. Get acceleration from new phase
                Q16_16 frame_acceleration = source.acceleration(phase_q16);

                // dt must be Q16.16 for physics calculations
                int32_t dt_chunk_q16_16 = (static_cast<int32_t>(chunk) << 16) / MILLIS_PER_SECOND;
                dt_chunk_q16_16 = constrain(dt_chunk_q16_16, 0, 1 << 16); // Clamp to max 1.0 sec

                // 3. Apply acceleration
                velocity += scale_i32_by_q16_16(frame_acceleration, dt_chunk_q16_16);

                // 4. Apply damping
                int32_t retention_this_chunk_q16 = pow_q16(retention_per_sec, dt_chunk_q16_16);
                velocity = scale_i32_by_q16_16(velocity, retention_this_chunk_q16);

                // 5. Update position
                position += scale_i32_by_q16_16(velocity, dt_chunk_q16_16);

                // 6. Apply boundary policies
                policy.apply(position, velocity);
            }
        }

        /**
         * @brief Gets the current position of the signal.
         * @return The integer part of the signal's position, not the full Q16.16 value.
         */
        Value getValue() const {
            return position >> 16;
        }
    };

    using LinearSignal = Signal<LinearPolicy>;
    using BoundedSignal = Signal<ClampPolicy>;
    /// A signal whose position wraps, suitable for angles measured in turns (1.0 turn = 65536 units).
    using AngularSignal = Signal<WrapPolicy>;
}

#endif //LED_SEGMENTS_EFFECTS_MAPPERS_VALUEMAPPERS_H
