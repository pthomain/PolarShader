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

#ifndef LED_SEGMENTS_EFFECTS_DECORATORS_VIEWPORTDECORATOR_H
#define LED_SEGMENTS_EFFECTS_DECORATORS_VIEWPORTDECORATOR_H

#include "base/Decorators.h"
#include "../mappers/ValueMappers.h"
#include "../utils/MathUtils.h"

namespace LEDSegments {
    /**
     * @class ViewPortDecorator
     * @brief Manages the "camera" over the Cartesian noise field, handling zoom and translation.
     *        Zoom is controlled by a logarithmic scale signal for perceptually stable zooming.
     */
    class ViewPortDecorator : public CartesianDecorator {
        LinearSignal positionX;
        LinearSignal positionY;
        BoundedSignal logScale; // Signal's value is log2(scale), conceptually a Q8.8 value.

        // Cache the scale factor to avoid re-computation when the log-scale hasn't changed.
        int16_t lastQuantizedLogScale = INT16_MIN;
        int32_t scaleFactor = 1 << 16; // Default to 1.0 in Q16.16

    public:
        ViewPortDecorator(
            LinearSignal x,
            LinearSignal y,
            BoundedSignal logScaleSignal
        ) : positionX(std::move(x)),
            positionY(std::move(y)),
            logScale(std::move(logScaleSignal)) {
        }

        void advanceFrame(unsigned long timeInMillis) override {
            positionX.advanceFrame(timeInMillis);
            positionY.advanceFrame(timeInMillis);
            logScale.advanceFrame(timeInMillis);

            // Quantize the log-scale value to prevent cache misses from minor noise jitter.
            // A Q8.4 quantization (16 levels per integer) is a good balance.
            int16_t currentLogScale_q8_8 = (int16_t) logScale.getValue();
            int16_t quantizedLogScale = (currentLogScale_q8_8 >> 4) << 4;

            if (quantizedLogScale != lastQuantizedLogScale) {
                lastQuantizedLogScale = quantizedLogScale;

                // Defensively clamp logScale to a safe range (-8.0 to +8.0) before use.
                int16_t clampedLogScale = constrain(quantizedLogScale, -8 << 8, 8 << 8);

                // Convert the Q8.8 log2 scale to a Q16.16 linear scale factor.
                int32_t linearScale_q16 = log2_to_linear_q16(clampedLogScale);

                // Clamp to a minimum scale to prevent extreme zoom-out. 0.25x is a reasonable minimum.
                static constexpr int32_t MIN_LINEAR_SCALE_Q16 = (1 << 16) / 4; // 0.25 in Q16.16
                if (linearScale_q16 < MIN_LINEAR_SCALE_Q16) {
                    linearScale_q16 = MIN_LINEAR_SCALE_Q16;
                }

                // The scaleFactor is the reciprocal (inverse) of the linear scale.
                scaleFactor = ((uint64_t) 1 << 32) / linearScale_q16;
            }
        }

        CartesianLayer operator()(const CartesianLayer &layer) const override {
            return [this, layer](int32_t x, int32_t y, unsigned long t) {
                int32_t sx = scale_i32_by_q16_16(x, scaleFactor);
                int32_t sy = scale_i32_by_q16_16(y, scaleFactor);

                return layer(
                    sx + positionX.getValue(),
                    sy + positionY.getValue(),
                    t
                );
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_VIEWPORTDECORATOR_H
