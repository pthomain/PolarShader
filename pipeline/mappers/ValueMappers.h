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

namespace LEDSegments {
    /**
    * AbsoluteValue:
    *   Stateless or internally smoothed
    *   Frame-rate independent
    *   Idempotent
    *   Safe to sample anywhere in the pipeline
    *
    *   Examples:
    *       Zoom level
    *       Palette index
    *       Symmetry count
    *       Vortex strength
    *       Brightness envelope
    */
    using AbsoluteValue = std::function<uint16_t(unsigned long)>;

    /**
    * DeltaValue (motion):
    *   Represents change, not position
    *   Must be integrated by the consumer
    *   Frame-rate dependent unless explicitly normalized
    *   Order-sensitive
    *
    *   Examples:
    *       Rotation speed
    *       Translation velocity
    *       Angular drift
    *       Phase accumulation
    *       Flow-field motion
    */
    using DeltaValue = std::function<int32_t(unsigned long)>;

    /**
     * @brief Creates a mapper that returns a constant value, scaled from a 0-1000 input range.
     * @param value_0_1000 The desired value in the range 0-1000. Values above 1000 will be clamped.
     */
    inline AbsoluteValue ConstantValue(uint16_t value_0_1000) {
        uint16_t clampedValue = min(value_0_1000, (uint16_t) 1000);
        uint16_t mappedValue = (uint32_t) clampedValue * 65535 / 1000;
        return [mappedValue](unsigned long /*timeInMillis*/) {
            return mappedValue;
        };
    }

    /**
     * @brief Creates a mapper that returns a value increasing linearly with time.
     * @param scale The rate of change.
     */
    inline AbsoluteValue TimeScaledValue(uint16_t scale) {
        return [scale](unsigned long timeInMillis) {
            // The result is shifted to keep the rate of change manageable.
            return (uint16_t) ((timeInMillis * scale) >> 8);
        };
    }

    /**
     * @brief Creates a mapper that returns a 1D Perlin noise value that evolves over time.
     * @param scale Controls the "speed" of travel through the noise field.
     * @param magnitude_0_1000 The output amplitude (0-1000), mapped to 0-65535. Defaults to 1000.
     * @param seed An offset into the noise field, allowing for different random seeds.
     */
    inline AbsoluteValue NoiseValue(uint16_t freq_0_1000,
                                    uint16_t magnitude_0_1000 = 1000) {
        // Map inputs to usable ranges
        uint16_t magnitude = map(min(magnitude_0_1000, 1000), 0, 1000, 0, 65535);
        uint16_t freq = map(min(freq_0_1000, 1000), 0, 1000, 1, 50);

        return [freq, magnitude, currentValue = uint16_t(32768)](unsigned long timeInMillis) mutable -> uint16_t {
            // Slow down time so changes are gradual
            uint32_t scaledTime = (timeInMillis * freq) >> 4;

            uint16_t target = inoise16(scaledTime);
            target = scale16(target, magnitude); // scale amplitude

            // Smooth step with IIR filter
            currentValue += ((int32_t) target - currentValue) >> 4;

            return currentValue;
        };
    }

    inline DeltaValue NoiseDelta(
        uint16_t magnitude_0_1000
    ) {
        // Map UI inputs
        uint16_t magnitude = map(min(magnitude_0_1000, 1000), 0, 1000, 0, 32768);

        int32_t velocity = 0;

        return [magnitude, velocity](unsigned long timeInMillis) mutable -> int32_t {
            uint32_t t = (timeInMillis) >> 4;

            // Signed noise target
            int32_t target = (int32_t) inoise16(t) - 32768;

            // Amplitude control
            target = (target * magnitude) >> 15;

            // Temporal smoothing (inertia)
            velocity += (target - velocity) >> 3;

            return velocity;
        };
    }

    inline DeltaValue ConstantDelta(
        uint16_t magnitude_0_1000 = 1000
    ) {
        // Map UI inputs
        uint16_t magnitude = map(min(magnitude_0_1000, 1000), 0, 1000, 0, 32768);

        return [ magnitude](unsigned long timeInMillis) mutable -> int32_t {
            return magnitude;
        };
    }
}

#endif //LED_SEGMENTS_EFFECTS_MAPPERS_VALUEMAPPERS_H
