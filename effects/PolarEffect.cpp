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

#include "FastLED.h"
#include "PolarEffect.h"

namespace LEDSegments {
    static const PolarEffectFactory factoryInstance;
    RenderableFactoryRef<CRGB> PolarEffect::factory = &factoryInstance;

    int32_t globalPositionX = random16();
    int32_t globalPositionY = random16();
    int32_t prevVx = 0;
    int32_t prevVy = 0;

    int32_t globalAngle = 0;
    int32_t angularVelocity = 0;

    void calculateAngle(unsigned long timeInMillis) {
        uint16_t noiseAngle = inoise16(timeInMillis << 5);

        // Map noise to signed angular velocity
        auto targetAngularVelocity = (int16_t) (noiseAngle - 32768);

        const uint8_t angularSpeedShift = 3; //lower is faster
        targetAngularVelocity >>= angularSpeedShift;

        angularVelocity = (angularVelocity + targetAngularVelocity) >> 1; //smoothing

        globalAngle += angularVelocity;
    }

    void calculatePosition(unsigned long timeInMillis) {
        uint16_t directionAngle = inoise16(globalPositionX, globalPositionY, timeInMillis << 5);
        int32_t vx = cos16(directionAngle);
        int32_t vy = sin16(directionAngle);

        const uint8_t speedShift = 4; // controls speed, lower is faster
        vx >>= speedShift;
        vy >>= speedShift;

        // Integrate
        vx = (prevVx + vx) >> 1; // >>1 adds inertia to smooth motion
        vy = (prevVy + vy) >> 1;

        globalPositionX += vx;
        globalPositionY += vy;

        prevVx = vx;
        prevVy = vy;
    }

    void PolarEffect::fillSegmentArray(
        CRGB *segmentArray,
        uint16_t segmentSize,
        uint16_t segmentIndex,
        fract16 progress,
        unsigned long timeInMillis
    ) {
        // calculateAngle(timeInMillis);
        calculatePosition(timeInMillis);

        for (uint16_t pixelIndex = 0; pixelIndex < segmentSize; ++pixelIndex) {
            auto [angle, radius] = context.polarCoordsMapper(pixelIndex);

            fillPolar(
                segmentArray,
                pixelIndex,
                angle + globalAngle,
                radius,
                timeInMillis,
                globalPositionX,
                globalPositionY,
                {
                    colourNoiseLayer
                }
            );
        }
    }
}
