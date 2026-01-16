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

#ifndef LED_SEGMENTS_PIPELINE_SIGNALS_SIGNAL_H
#define LED_SEGMENTS_PIPELINE_SIGNALS_SIGNAL_H

#include <type_traits>
#include "Waveforms.h"
#include "SignalPolicies.h"
#include "WaveformSources.h"
#include "../utils/MathUtils.h"
#include "../utils/FixMathUtils.h"
#include "../utils/Units.h"

namespace LEDSegments {

    template<typename Policy>
    class Signal;

    /// A signal that can move indefinitely in a linear fashion (no bounds).
    /// Use for offsets or camera moves where wrap/clamp behavior is not desired.
    using LinearSignal = Signal<LinearPolicy>;

    /// A signal whose value is clamped within a specified min/max range.
    /// Use for zoom/radius controls that must stay within safe limits.
    using BoundedSignal = Signal<ClampPolicy>;

    /// A signal whose value wraps around, like an angle (0-360 degrees). Defaults to 65536 units (AngleTurns16).
    /// Use for anything periodic (rotation, hue phase) where modulo wrap is desired.
    using AngularSignal = Signal<WrapPolicy>;

    /**
     * @brief A time-varying value that evolves based on a waveform and physics simulation.
     *
     * The Signal class represents a dynamic value (like position) that changes over time.
     * It simulates physical properties like velocity and acceleration, which are influenced
     * by a provided waveform (the 'source'). The signal also has a 'retention' property,
     * which acts like friction or drag, causing the velocity to decay over time.
     *
     * Policies define boundary behavior:
     *  - LinearPolicy: unbounded.
     *  - ClampPolicy: clamps to min/max and zeroes velocity on hit.
     *  - WrapPolicy: wraps at wrap_units<<16; AngularSignal defaults to 65536 units (AngleTurns16 domain).
     *
     * Edge cases and units:
     *  - advanceFrame clamps deltaTime to 200 ms and integrates in 100 ms chunks for stability.
     *  - Phase velocity is “AngleTurns16 per second” scaled Q16.16; 1.0 == 1/65536 of a full wrap per second.
     *  - Negative velocities wrap via unsigned addition.
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
        using Value = int32_t;

    private:
        Units::SignalQ16_16 position;
        Units::SignalQ16_16 velocity = Units::SignalQ16_16(0);
        WaveformSources::WaveformSource source;
        Units::PhaseTurnsUQ16_16 phase_q16 = 0;
        Units::TimeMillis lastTime = 0;
        Policy policy;

        friend Units::AngleTurns16 asAngleTurns(const AngularSignal& s);

    public:
        /**
         * @brief Constructs a new Signal.
         * @param initialPosition The starting value of the signal.
         * @param waveformSource A function that provides acceleration to the signal over time.
         * @param policy_args Arguments to be forwarded to the policy's constructor.
         */
        template<typename... PolicyArgs>
        Signal(Value initialPosition = 0,
               WaveformSources::WaveformSource waveformSource = WaveformSources::Constant(0),
               PolicyArgs... policy_args)
            : position(initialPosition),
              source(std::move(waveformSource)),
              policy(policy_args...) {
            phase_q16 = (Units::PhaseTurnsUQ16_16) random16() << 16;
        }

        /**
         * @brief Constructs a new Signal from a Q16.16 initial value (allows fractional starts).
         */
        template<typename... PolicyArgs>
        Signal(Units::SignalQ16_16 initialPosition,
               WaveformSources::WaveformSource waveformSource = WaveformSources::Constant(0),
               PolicyArgs... policy_args)
            : position(initialPosition),
              source(std::move(waveformSource)),
              policy(policy_args...) {
            phase_q16 = (Units::PhaseTurnsUQ16_16) random16() << 16;
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
        void advanceFrame(Units::TimeMillis currentTime) {
            if (lastTime == 0) {
                lastTime = currentTime;
                return;
            }
            Units::TimeMillis deltaTime = currentTime - lastTime;
            lastTime = currentTime;
            if (deltaTime == 0) return;

            const Units::TimeMillis MAX_DELTA_TIME = 200;
            if (deltaTime > MAX_DELTA_TIME) deltaTime = MAX_DELTA_TIME;

            const Units::TimeMillis CHUNK_SIZE_MS = 100;
            Units::TimeMillis remaining_dt = deltaTime;

            while (remaining_dt > 0) {
                Units::TimeMillis chunk = (remaining_dt < CHUNK_SIZE_MS) ? remaining_dt : CHUNK_SIZE_MS;
                remaining_dt -= chunk;

                Units::SignalQ16_16 dt_q16 = millisToQ16_16(chunk);

                // Advance phase using wrapping multiplication
                Units::PhaseVelAngleUnitsQ16_16 phase_velocity_q16 = source.phaseVelocity(phase_q16);
                Units::RawSignalQ16_16 phase_advance = mul_q16_16_wrap(phase_velocity_q16, dt_q16).asRaw();
                phase_q16 += static_cast<uint32_t>(phase_advance);

                // Apply acceleration to velocity using saturating multiplication
                Units::SignalQ16_16 acceleration = source.acceleration(phase_q16);
                Units::SignalQ16_16 dv = mul_q16_16_sat(acceleration, dt_q16);
                velocity = Units::SignalQ16_16::fromRaw(add_sat_q16_16(velocity.asRaw(), dv.asRaw()));

                // Apply velocity to position
                Units::SignalQ16_16 dp = mul_q16_16_sat(velocity, dt_q16);
                Units::RawSignalQ16_16 next_position_raw;
                if constexpr (std::is_same_v<Policy, WrapPolicy>) {
                    next_position_raw = add_wrap_q16_16(position.asRaw(), dp.asRaw());
                } else {
                    next_position_raw = add_sat_q16_16(position.asRaw(), dp.asRaw());
                }
                position = Units::SignalQ16_16::fromRaw(next_position_raw);

                policy.apply(position, velocity);
            }
        }

        /**
         * @brief Gets the integer part of the signal's current position.
         * @return The integer part of the signal's current position.
         */
        Value getValue() const { return position.asInt(); }

        /**
         * @brief Gets the raw Q16.16 value of the signal's current position.
         * @return The raw Q16.16 value.
         */
        Units::RawSignalQ16_16 getRawValue() const { return position.asRaw(); }
    };

    /**
     * @brief Gets the current value of an AngularSignal as an angle in turns (0-65535).
     * Assumes the AngularSignal uses a WrapPolicy of 65536 units (AngleTurns16 domain).
     */
    inline Units::AngleTurns16 asAngleTurns(const AngularSignal& s) {
        return static_cast<Units::AngleTurns16>(static_cast<uint32_t>(s.getRawValue()) >> 16);
    }
}

#endif //LED_SEGMENTS_PIPELINE_SIGNALS_SIGNAL_H
