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

#ifndef POLARSHADER_FABRICDISPLAYSPEC_H
#define POLARSHADER_FABRICDISPLAYSPEC_H

#include "MatrixDisplaySpec.h"

namespace PolarShader {
    class FabricDisplaySpec : public MatrixDisplaySpec {
    public:
        static constexpr int LED_PIN = D1;
        static constexpr EOrder RGB_ORDER = GRB;
        static constexpr uint16_t WIDTH = 20;
        static constexpr uint16_t HEIGHT = 20;
        static constexpr uint16_t SUBSAMPLE = 1;

        uint16_t displayWidth() const override { return WIDTH; }
        uint16_t displayHeight() const override { return HEIGHT; }
        uint16_t subsample() const override { return SUBSAMPLE; }

        PolarCoords toPolarCoords(uint16_t pixelIndex) const override {
            uint16_t row = pixelIndex / WIDTH;
            uint16_t col = pixelIndex % WIDTH;

            // Serpentine: odd rows are wired right-to-left
            if (row & 1) {
                col = (WIDTH - 1) - col;
            }

            uint16_t linearIndex = row * WIDTH + col;
            return MatrixDisplaySpec::toPolarCoords(linearIndex);
        }
    };
}
#endif //POLARSHADER_FABRICDISPLAYSPEC_H