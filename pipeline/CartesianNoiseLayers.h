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

#ifndef LED_SEGMENTS_SPECS_POLARLAYERS_H
#define LED_SEGMENTS_SPECS_POLARLAYERS_H

#include "utils/MathUtils.h"

namespace LEDSegments {
    /**
     * @brief A base noise layer that samples 2D Perlin noise.
     * @param x The signed 32-bit x-coordinate.
     * @param y The signed 32-bit y-coordinate.
     * @param timeInMillis The current animation time.
     * @return A 16-bit scalar value (0-255) to be used as a palette index.
     *
     * This function performs the final, critical conversion from the pipeline's
     * standard signed Cartesian space to the unsigned domain required by inoise16.
     */
    inline uint16_t noiseLayer(
        uint32_t x,
        uint32_t y,
        unsigned long timeInMillis
    ) {
        uint8_t speed = 10;
        uint32_t t = timeInMillis * speed;
        return inoise16(x, y, t);
    }

    /**
     * @brief A noise layer that generates Fractal Brownian Motion (fBm).
     * This creates a more detailed, multi-layered noise pattern by summing
     * multiple "octaves" of noise at different frequencies and amplitudes.
     * @note The coordinate doubling (`x <<= 1`) can lead to rapid growth. The
     * number of octaves is intentionally limited to prevent overflow of the
     * 32-bit coordinate space, especially after upstream scaling decorators.
     */
    inline uint16_t fBmLayer(
        uint32_t x,
        uint32_t y,
        unsigned long timeInMillis
    ) {
        static fl::u8 octaves = 4;
        uint8_t speed = 30;
        uint32_t t = timeInMillis * speed;

        uint16_t r = 0;
        uint16_t amplitude = U16_HALF;

        for (int o = 0; o < octaves; o++) {
            r += (inoise16(x, y, t) * amplitude) >> 16;
            x <<= 1;
            y <<= 1;
            amplitude >>= 1;
        }

        return r;
    }

    /**
     * @brief A noise layer that generates a turbulence pattern.
     * It uses the absolute value of signed noise to create sharp creases,
     * resulting in a "flame-like" or "marbled" appearance.
     */
    inline uint16_t turbulenceLayer(
        uint32_t x,
        uint32_t y,
        unsigned long timeInMillis
    ) {
        constexpr uint8_t speed = 10;
        uint32_t t = timeInMillis * speed;

        int16_t r = (int16_t) inoise16(x, y, t) - 32768;
        uint16_t mag = (uint16_t) (r ^ (r >> 15)) - (uint16_t) (r >> 15);

        return mag << 1;
    }

    /**
     * @brief A noise layer that generates a ridged multifractal pattern.
     * This is effectively the inverse of turbulence, creating bright ridges
     * on a dark background, suitable for lightning or vein-like patterns.
     */
    inline uint16_t ridgedLayer(
        uint32_t x,
        uint32_t y,
        unsigned long timeInMillis
    ) {
        constexpr uint8_t speed = 10;
        uint32_t t = timeInMillis * speed;

        int16_t r = (int16_t) inoise16(x, y, t) - 32768;
        uint16_t mag = (uint16_t) (r ^ (r >> 15)) - (uint16_t) (r >> 15);
        mag = min<uint16_t>(mag, 32767);

        uint16_t inverted = 65535 - (mag << 1);
        return inverted;
    }
}
#endif //LED_SEGMENTS_SPECS_POLARLAYERS_H
