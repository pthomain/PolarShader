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

#ifndef LED_SEGMENTS_SPECS_CAMERARIG_H
#define LED_SEGMENTS_SPECS_CAMERARIG_H

#include "polar/pipeline/mappers/ValueMappers.h"
#include "polar/pipeline/utils/MathUtils.h"

namespace LEDSegments {
    class CameraRig {
        LinearSignal posX;
        LinearSignal posY;
        BoundedSignal logZoom;
        AngularSignal rotationSignal;
        BoundedSignal vortexSignal;

        int16_t lastQuantizedLogScale = INT16_MIN;
        int32_t cachedInverseScale = 1 << 16; // Default to 1.0 in Q16.16

    public:
        CameraRig(
            LinearSignal x,
            LinearSignal y,
            BoundedSignal zoom,
            AngularSignal rotation,
            BoundedSignal vortex
        ) : posX(std::move(x)),
            posY(std::move(y)),
            logZoom(std::move(zoom)),
            rotationSignal(std::move(rotation)),
            vortexSignal(std::move(vortex)) {
        }

        void advanceFrame(unsigned long t) {
            posX.advanceFrame(t);
            posY.advanceFrame(t);
            logZoom.advanceFrame(t);
            rotationSignal.advanceFrame(t);
            vortexSignal.advanceFrame(t);

            // The BoundedSignal for logZoom is configured to stay within a range that
            // safely fits in an int16_t, so this cast is safe.
            int16_t currentLogScale_q8_8 = (int16_t) logZoom.getValue();

            // Quantize to prevent cache misses from noise jitter
            int16_t quantizedLogScale = (currentLogScale_q8_8 >> 4) << 4;

            if (quantizedLogScale != lastQuantizedLogScale) {
                lastQuantizedLogScale = quantizedLogScale;

                int16_t clampedLogScale = constrain(quantizedLogScale, -8 << 8, 8 << 8);
                int32_t linearScale_q16 = log2_to_linear_q16(clampedLogScale);

                static constexpr int32_t MIN_LINEAR_SCALE_Q16 = (1 << 16) / 4; // 0.25x
                if (linearScale_q16 < MIN_LINEAR_SCALE_Q16) {
                    linearScale_q16 = MIN_LINEAR_SCALE_Q16;
                }

                cachedInverseScale = ((uint64_t) 1 << 32) / linearScale_q16;
            }
        }

        // World-space camera state
        int32_t positionX() const { return posX.getValue(); }
        int32_t positionY() const { return posY.getValue(); }

        // Log2 scale, returned as an int32_t but should be interpreted as Q8.8
        int32_t logScale() const { return logZoom.getValue(); }

        // Inverse linear scale (Q16.16) for coordinate scaling
        int32_t inverseLinearScale() const { return cachedInverseScale; }

        // Angular rotation (uint16 wrap)
        uint16_t rotation() const { return rotationSignal.getValue(); }

        // Absolute vortex strength (signed Q16.16 turns)
        int32_t vortex() const { return vortexSignal.getValue(); }
    };
} // namespace LEDSegments

#endif //LED_SEGMENTS_SPECS_CAMERARIG_H
