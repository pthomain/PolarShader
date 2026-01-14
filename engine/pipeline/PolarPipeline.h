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

#ifndef LED_SEGMENTS_EFFECTS_DECORATORS_POLARPIPELINE_H
#define LED_SEGMENTS_EFFECTS_DECORATORS_POLARPIPELINE_H

#include "transforms/ViewPortTransform.h"
#include <type_traits>
#include "PolarUtils.h"
#include "utils/NoiseUtils.h"
#include "ViewPort.h"

namespace LEDSegments {
    /**
     * @brief Manages the rendering pipeline for polar effects.
     *
     * The PolarPipeline is responsible for chaining together a series of transforms
     * to transform coordinates and generate the final colour for each pixel. It handles
     * both Cartesian and Polar coordinate transforms.
     */
    class PolarPipeline {
        ViewPortTransform viewPortTransform;
        fl::vector<CartesianTransform *> cartesianTransforms;
        fl::vector<PolarTransform *> polarTransforms;

    public:
        explicit PolarPipeline(ViewPort &camera) : viewPortTransform(camera) {
        }

        template<typename T, typename = std::enable_if_t<std::is_base_of<PolarTransform, T>::value> >
        void addPolarTransform(T *transform) {
            polarTransforms.push_back(transform);
        }

        template<typename T, typename = std::enable_if_t<std::is_base_of<CartesianTransform, T>::value> >
        void addCartesianTransform(T *transform) {
            cartesianTransforms.push_back(transform);
        }

        void clear() {
            cartesianTransforms.clear();
            polarTransforms.clear();
        }

        void advanceFrame(unsigned long timeInMillis) {
            viewPortTransform.advanceFrame(timeInMillis);
            for (auto *transform: cartesianTransforms) {
                transform->advanceFrame(timeInMillis);
            }
            for (auto *transform: polarTransforms) {
                transform->advanceFrame(timeInMillis);
            }
        }

        ColourLayer build(const NoiseLayer &sourceLayer, const CRGBPalette16 &palette) {
            CartesianLayer adaptedSource = [sourceLayer](int32_t x, int32_t y, unsigned long t) {
                return sourceLayer(
                    x + NOISE_DOMAIN_OFFSET,
                    y + NOISE_DOMAIN_OFFSET,
                    t
                );
            };

            // Apply viewport first, so subsequent cartesian transforms (e.g., domain warp)
            // are not scaled by zoom.
            CartesianLayer currentCartesian = viewPortTransform(adaptedSource);

            // Apply Cartesian transforms in reverse order for correct functional composition.
            for (auto it = cartesianTransforms.rbegin(); it != cartesianTransforms.rend(); ++it) {
                currentCartesian = (**it)(currentCartesian);
            }

            // Convert the Cartesian layer to a Polar layer.
            PolarLayer currentPolar = [layer = currentCartesian
                    ](uint16_t angle_turns, fract16 radius, unsigned long t) {
                auto [x, y] = cartesianCoords(angle_turns, radius);
                return layer(x, y, t);
            };

            // Apply Polar transforms in reverse order.
            for (auto it = polarTransforms.rbegin(); it != polarTransforms.rend(); ++it) {
                currentPolar = (**it)(currentPolar);
            }

            // Final stage: sample the noise value and map it to a color from the palette.
            return [palette, layer = currentPolar](uint16_t angle_turns, fract16 radius, unsigned long t) {
                uint16_t value = normaliseNoise16(layer(angle_turns, radius, t));
                uint8_t phase = 0; //t / 8;
                uint8_t index = map16_to_8(value) + phase;
                return ColorFromPalette(palette, index, 255, LINEARBLEND);
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_POLARPIPELINE_H
