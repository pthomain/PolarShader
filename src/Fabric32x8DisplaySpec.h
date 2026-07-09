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

#ifndef POLARSHADER_FABRIC32X8DISPLAYSPEC_H
#define POLARSHADER_FABRIC32X8DISPLAYSPEC_H

#include "MatrixDisplaySpec.h"

namespace PolarShader {
    // 32-wide by 8-tall panel sharing FabricDisplaySpec's electrical wiring
    // (D1 / GRB) but a vertical serpentine layout: the strip snakes in columns
    // of 8 — even columns run top-to-bottom, odd columns bottom-to-top.
    class Fabric32x8DisplaySpec : public MatrixDisplaySpec {
    public:
        static constexpr int LED_PIN = D1;
        static constexpr EOrder RGB_ORDER = GRB;
        static constexpr uint16_t WIDTH = 32;
        static constexpr uint16_t HEIGHT = 8;
        static constexpr uint16_t SUBSAMPLE = 1;

        uint16_t displayWidth() const override { return WIDTH; }
        uint16_t displayHeight() const override { return HEIGHT; }
        uint16_t subsample() const override { return SUBSAMPLE; }

        // 32x8 is a 4:1 panel: centre-crop so polar/UV effects stay undistorted.
        bool centerCrop() const override { return true; }

        PolarCoords toPolarCoords(uint16_t pixelIndex) const override {
            uint16_t col = pixelIndex / HEIGHT;
            uint16_t p = pixelIndex % HEIGHT;

            // Vertical serpentine: even columns run top-to-bottom, odd columns
            // bottom-to-top.
            uint16_t row = (col & 1) ? (HEIGHT - 1 - p) : p;

            uint16_t linearIndex = row * WIDTH + col;
            return MatrixDisplaySpec::toPolarCoords(linearIndex);
        }

        RenderPoint toRenderPoint(uint16_t pixelIndex) const override {
            uint16_t col = pixelIndex / HEIGHT;
            uint16_t p = pixelIndex % HEIGHT;

            // Vertical serpentine: even columns run top-to-bottom, odd columns
            // bottom-to-top. Raster patterns need visual x/y, so recover the
            // column and row this physical LED occupies.
            uint16_t row = (col & 1) ? (HEIGHT - 1 - p) : p;

            uint16_t linearIndex = row * WIDTH + col;
            RenderPoint point = makePolarRenderPoint(MatrixDisplaySpec::toPolarCoords(linearIndex));
            point.raster.valid = true;
            point.raster.index = linearIndex;
            point.raster.x = col;
            point.raster.y = row;
            point.raster.width = WIDTH;
            point.raster.height = HEIGHT;
            return point;
        }
    };
}
#endif //POLARSHADER_FABRIC32X8DISPLAYSPEC_H
