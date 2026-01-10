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
#include "polar/pipeline/utils/MathUtils.h"

namespace LEDSegments {
    class ViewPortDecorator : public CartesianDecorator {
        // Translation state
        int32_t prevVx = 0;
        int32_t prevVy = 0;
        int32_t globalPositionX = random16();
        int32_t globalPositionY = random16();

        // Mappers
        AbsoluteValue scaleMapper;
        DeltaValue amplitudeMapper;
        DeltaValue directionMapper;

        // Scale constants
        static constexpr uint16_t MIN_SCALE = 128; // 0.5x
        static constexpr uint16_t MAX_SCALE = 1024; // 4.0x
        static constexpr uint8_t DOMAIN_SCALE_SHIFT = 8;

        // Translation constants
        static constexpr uint8_t SPEED_SHIFT = 2;
        static constexpr int16_t MAX_AMPLITUDE = 1024;

    public:
        ViewPortDecorator(
            AbsoluteValue scaleMapper,
            AbsoluteValue amplitudeMapper,
            AbsoluteValue directionMapper
        ) : scaleMapper(std::move(scaleMapper)),
            amplitudeMapper(std::move(amplitudeMapper)),
            directionMapper(std::move(directionMapper)) {
        }

        void advanceFrame(unsigned long timeInMillis) override {
            // Advance translation
            uint16_t amplitude = map(amplitudeMapper(timeInMillis), 0, 65535, 0, MAX_AMPLITUDE);
            fract16 directionAngle = directionMapper(timeInMillis);
            int32_t vx = scale_i16_by_f16(cos16(directionAngle), amplitude);
            int32_t vy = scale_i16_by_f16(sin16(directionAngle), amplitude);
            vx = (prevVx + vx) >> 1;
            vy = (prevVy + vy) >> 1;
            globalPositionX += (vx >> SPEED_SHIFT);
            globalPositionY += (vy >> SPEED_SHIFT);
            prevVx = vx;
            prevVy = vy;
        }

        CartesianLayer operator()(const CartesianLayer &layer) const override {
            return [this, layer](int32_t x, int32_t y, unsigned long t) {
                // Map scale as frequency (larger = zoom out)
                uint16_t scale = map(scaleMapper(t), 0, 65535, MIN_SCALE, MAX_SCALE);

                // Prevent divide-by-zero / collapse
                scale = max<uint16_t>(scale, MIN_SCALE);

                // Apply scale BEFORE translation so translation speed is scale-independent
                int32_t sx = ((int64_t) x << DOMAIN_SCALE_SHIFT) / scale;
                int32_t sy = ((int64_t) y << DOMAIN_SCALE_SHIFT) / scale;

                return layer(
                    sx + globalPositionX,
                    sy + globalPositionY,
                    t
                );
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_VIEWPORTDECORATOR_H
