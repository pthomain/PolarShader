//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

/*
 * This file is part of PolarShader.
 *
 * PolarShader is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PolarShader is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PolarShader. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef POLARSHADER_DEFAULTPOLARDISPLAYSPEC_H
#define POLARSHADER_DEFAULTPOLARDISPLAYSPEC_H

#include "display/PolarDisplaySpec.h"
#include "renderer/pipeline/maths/Maths.h"

namespace PolarShader {
    class RoundDisplaySpec : public PolarDisplaySpec {
    public:
        static constexpr int LED_PIN = 9;
        static constexpr EOrder RGB_ORDER = GRB;
        static constexpr uint16_t NB_SEGMENTS = 9;
        static constexpr uint16_t NB_LEDS = 241;

        uint16_t numSegments() const override {
            return NB_SEGMENTS;
        }

        uint16_t nbLeds() const override {
            return NB_LEDS;
        }

        uint16_t segmentSize(uint16_t segmentIndex) const override {
            switch (segmentIndex) {
                case 0: return 1; // Center
                case 1: return 8;
                case 2: return 12;
                case 3: return 16;
                case 4: return 24;
                case 5: return 32;
                case 6: return 40;
                case 7: return 48;
                case 8: return 60; // Outermost ring
                default: return 0;
            }
        }

        PolarCoords toPolarCoords(uint16_t pixelIndex) const override {
            uint16_t cumulativePixels = 0;

            for (uint16_t segmentIndex = 0; segmentIndex < NB_SEGMENTS; ++segmentIndex) {
                const uint16_t currentSegmentSize = segmentSize(segmentIndex);
                if (pixelIndex < cumulativePixels + currentSegmentSize) {
                    // The pixel is in this segment.
                    const uint16_t pixelInSegment = pixelIndex - cumulativePixels;

                    // The angle is the pixel's proportional position within its ring.
                    // It's a uint16_t from 0 to 65535, representing 0 to 2PI radians.
                    // For the center pixel (segment 0), the angle is 0.
                    uint32_t angle_step_raw = currentSegmentSize > 1 ? (0x10000u / currentSegmentSize) : 0;
                    uint32_t angle_raw = static_cast<uint32_t>(pixelInSegment) * angle_step_raw;
                    f16 angle(angle_raw & 0xFFFFu);

                    f16 radius = toF16(
                        segmentIndex,
                        NB_SEGMENTS > 1 ? NB_SEGMENTS - 1 : 1
                    );

                    return {angle, radius};
                }

                cumulativePixels += currentSegmentSize;
            }

            // This should not be reached if pixelIndex is valid.
            return {0, 0};
        }
    };
}
#endif //POLARSHADER_DEFAULTPOLARDISPLAYSPEC_H
