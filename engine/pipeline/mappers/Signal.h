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

#ifndef LED_SEGMENTS_EFFECTS_MAPPERS_SIGNAL_H
#define LED_SEGMENTS_EFFECTS_MAPPERS_SIGNAL_H

#include "Waveforms.h"
#include "SignalPolicies.h"
#include "../utils/MathUtils.h"

namespace LEDSegments {
    static constexpr uint16_t MILLIS_PER_SECOND = 1000;

    /**
     * @brief A time-varying value that evolves based on a waveform and physics simulation.
     *
     * The Signal class represents a dynamic value (like position) that changes over time.
     * It simulates physical properties like velocity and acceleration, which are influenced
     * by a provided waveform (the 'source'). The signal also has a 'retention' property,
     * which acts like friction or drag, causing the velocity to decay over time.
     *
     * The behavior of the signal at its boundaries is determined by a template 'Policy'.
     * For example, a signal can wrap around (like an angle), be clamped within a range,
     * or move linearly without bounds.
     *
     * @tparam Policy A policy class that defines how the signal behaves at its boundaries.
     *                Examples: LinearPolicy, ClampPolicy, WrapPolicy.
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
        fract16 retention_per_sec; //todo rename to friction
        Time lastTime = 0;
        Policy policy;

    public:
        /**
         * @brief Constructs a new Signal.
         * @param initialPosition The starting value of the signal.
         * @param waveformSource A function that provides acceleration to the signal over time.
         * @param retention_per_sec_0_1000 The percentage of velocity to retain per second (0-1000).
         *                                 Acts like friction. 900 means 90% retention per second.
         * @param policy_args Arguments to be forwarded to the policy's constructor.
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
            uint16_t constrained_retention = min((uint16_t) 999, retention_per_sec_0_1000);
            retention_per_sec = divide_u16_as_fract16(constrained_retention, 1000);
            phase_q16 = (uint32_t) random16() << 16;
        }

        /**
         * @brief Advances the signal's simulation by a given amount of time.
         *
         * This method updates the signal's position and velocity based on the elapsed time,
         * the waveform source, and the retention factor. The simulation is performed in
         * fixed-size chunks to maintain stability.
         *
         * @param currentTime The current time in milliseconds.
         */
        void advanceFrame(Time currentTime) {
            if (lastTime == 0) {
                lastTime = currentTime;
                return;
            }
            Time deltaTime = currentTime - lastTime;
            lastTime = currentTime;
            if (deltaTime == 0) return;

            const Time MAX_DELTA_TIME = 200;
            if (deltaTime > MAX_DELTA_TIME) deltaTime = MAX_DELTA_TIME;

            const Time CHUNK_SIZE_MS = 100;
            Time remaining_dt = deltaTime;

            while (remaining_dt > 0) {
                Time chunk = min(remaining_dt, CHUNK_SIZE_MS);
                remaining_dt -= chunk;

                int32_t phase_velocity_q16 = source.phaseVelocity(phase_q16);
                int64_t advance_q16 = ((int64_t) phase_velocity_q16 * chunk) / MILLIS_PER_SECOND;
                phase_q16 += (uint32_t) advance_q16;

                Q16_16 frame_acceleration = source.acceleration(phase_q16);
                int32_t dt_chunk_q16_16 = (static_cast<int32_t>(chunk) << 16) / MILLIS_PER_SECOND;
                dt_chunk_q16_16 = constrain(dt_chunk_q16_16, 0, 1 << 16);

                velocity += scale_i32_by_q16_16(frame_acceleration, dt_chunk_q16_16);
                int32_t retention_this_chunk_q16 = pow_q16(retention_per_sec, dt_chunk_q16_16);
                velocity = scale_i32_by_q16_16(velocity, retention_this_chunk_q16);
                position += scale_i32_by_q16_16(velocity, dt_chunk_q16_16);
                policy.apply(position, velocity);
            }
        }

        /**
         * @brief Gets the current value of the signal.
         * @return The integer part of the signal's current position.
         */
        Value getValue() const { return position >> 16; }
    };

    /// A signal that can move indefinitely in a linear fashion.
    using LinearSignal = Signal<LinearPolicy>;

    /// A signal whose value is clamped within a specified min/max range.
    using BoundedSignal = Signal<ClampPolicy>;

    /// A signal whose value wraps around, like an angle (0-360 degrees).
    using AngularSignal = Signal<WrapPolicy>;
}

#endif //LED_SEGMENTS_EFFECTS_MAPPERS_SIGNAL_H
