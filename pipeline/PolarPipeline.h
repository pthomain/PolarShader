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

#include "decorators/ViewPortDecorator.h"
#include <type_traits>
#include "PolarUtils.h"
#include "utils/NoiseUtils.h"

namespace LEDSegments {
    class PolarPipeline {
        ViewPortDecorator viewPortDecorator;
        fl::vector<FrameDecorator *> frameDecorators;
        fl::vector<PolarDecorator *> polarDecorators;

    public:
        PolarPipeline(
            LinearSignal positionX,
            LinearSignal positionY,
            BoundedSignal logScale
        ) : viewPortDecorator(
            std::move(positionX),
            std::move(positionY),
            std::move(logScale)
        ) {
            frameDecorators.push_back(&viewPortDecorator);
        }

        template<typename T, typename = std::enable_if_t<std::is_base_of<PolarDecorator, T>::value> >
        void addPolarDecorator(T *decorator) {
            frameDecorators.push_back(decorator);
            polarDecorators.push_back(decorator);
        }

        void clear() {
            frameDecorators.clear();
            polarDecorators.clear();
        }

        void advanceFrame(unsigned long timeInMillis) {
            for (auto *decorator: frameDecorators) {
                decorator->advanceFrame(timeInMillis);
            }
        }

        ColourLayer build(const NoiseLayer &sourceLayer, const CRGBPalette16 &palette) {
            CartesianLayer adaptedSource = [sourceLayer](int32_t x, int32_t y, unsigned long t) {
                return sourceLayer(
                    (uint32_t) x + NOISE_DOMAIN_OFFSET,
                    (uint32_t) y + NOISE_DOMAIN_OFFSET,
                    t
                );
            };

            CartesianLayer adjustedViewPort = viewPortDecorator(adaptedSource);

            PolarLayer currentPolar = [layer = adjustedViewPort](uint16_t angle_turns, fract16 radius, unsigned long t) {
                auto [x, y] = cartesianCoords(angle_turns, radius);
                return layer(x, y, t);
            };

            for (size_t i = 0; i < polarDecorators.size(); ++i) {
                currentPolar = (*polarDecorators[polarDecorators.size() - 1 - i])(currentPolar);
            }

            return [palette, layer = currentPolar](uint16_t angle_turns, fract16 radius, unsigned long t) {
                uint8_t index = map16_to_8(normaliseNoise16(layer(angle_turns, radius, t)));
                return ColorFromPalette(palette, index, 255, LINEARBLEND);
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_POLARPIPELINE_H
