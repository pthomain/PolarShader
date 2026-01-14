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

#ifndef LED_SEGMENTS_SPECS_VIEWPORT_H
#define LED_SEGMENTS_SPECS_VIEWPORT_H

#include "mappers/Signal.h"
#include "utils/MathUtils.h"

namespace LEDSegments {
    /**
     * @brief Defines the viewpoint for the rendering pipeline, controlling position (pan) and zoom.
     *
     * The ViewPort is responsible for managing the fundamental properties of a "camera" that
     * define the viewing window on a 2D plane. This includes the X/Y position and the zoom level.
     */
    class ViewPort {
        LinearSignal posX;
        LinearSignal posY;
        BoundedSignal logZoom;

        int16_t lastQuantizedLogScale = INT16_MIN;
        int32_t cachedInverseScale = 1 << 16;

    public:
        /**
         * @brief Constructs a new ViewPort.
         * @param x A LinearSignal that controls the camera's X position.
         * @param y A LinearSignal that controls the camera's Y position.
         * @param zoom A BoundedSignal that controls the camera's zoom level (logarithmic).
         */
        ViewPort(
            LinearSignal x,
            LinearSignal y,
            BoundedSignal zoom
        ) : posX(std::move(x)),
            posY(std::move(y)),
            logZoom(std::move(zoom)) {
        }

        /**
         * @brief Advances the camera's state in time.
         * @param t The current time in milliseconds.
         */
        void advanceFrame(unsigned long t) {
            posX.advanceFrame(t);
            posY.advanceFrame(t);
            logZoom.advanceFrame(t);

            // The BoundedSignal for logZoom is configured to stay within a range that
            // safely fits in an int16_t, so this cast is safe.
            int16_t currentLogScale_q8_8 = (int16_t) logZoom.getValue();

            // Quantize to prevent cache misses from noise jitter
            int16_t quantizedLogScale = (currentLogScale_q8_8 >> 4) << 4;

            if (quantizedLogScale != lastQuantizedLogScale) {
                lastQuantizedLogScale = quantizedLogScale;
                int16_t clampedLogScale = constrain(quantizedLogScale, -8 << 8, 8 << 8);
                int32_t linearScale_q16 = log2_to_linear_q16(clampedLogScale);

                static constexpr int32_t MAX_LINEAR_SCALE_Q16 = 1 << 16;
                static constexpr int32_t MIN_LINEAR_SCALE_Q16 = MAX_LINEAR_SCALE_Q16 / 16;
                if (linearScale_q16 < MIN_LINEAR_SCALE_Q16) linearScale_q16 = MIN_LINEAR_SCALE_Q16;
                if (linearScale_q16 > MAX_LINEAR_SCALE_Q16) linearScale_q16 = MAX_LINEAR_SCALE_Q16;

                cachedInverseScale = ((uint64_t) 1 << 32) / linearScale_q16;
            }
        }

        /// Returns the camera's X position in world-space.
        int32_t positionX() const { return posX.getValue(); }
        /// Returns the camera's Y position in world-space.
        int32_t positionY() const { return posY.getValue(); }

        /// Returns the inverse linear scale (Q16.16) for coordinate scaling.
        int32_t inverseLinearScale() const { return cachedInverseScale; }
    };
}

#endif //LED_SEGMENTS_SPECS_VIEWPORT_H
