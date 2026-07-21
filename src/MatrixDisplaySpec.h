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

#ifndef POLARSHADER_MATRIXDISPLAYSPEC_H
#define POLARSHADER_MATRIXDISPLAYSPEC_H

#include "display/PolarDisplaySpec.h"
#include "renderer/pipeline/maths/Maths.h"

namespace PolarShader {
    class MatrixDisplaySpec : public PolarDisplaySpec {
    public:
        virtual uint16_t displayWidth() const = 0;
        virtual uint16_t displayHeight() const = 0;
        virtual uint16_t subsample() const = 0;

        // When true, the polar/UV coordinate mapping preserves aspect ratio and
        // centre-crops (cover fit) on non-square panels instead of stretching
        // each axis independently. Raster automata are unaffected — they run
        // 1:1 on the native grid and never pass through toPolarCoords().
        virtual bool centerCrop() const { return false; }

        uint16_t matrixWidth() const { return displayWidth() / subsample(); }
        uint16_t matrixHeight() const { return displayHeight() / subsample(); }

        // Scale so the unit circle has a diameter equal to the matrix diagonal.
        // 1 / sqrt(2) in u0x16/s0x16 (~0.7071).
        static constexpr uint16_t DIAGONAL_SCALE_U0X16 = 46341u;

        uint16_t numSegments() const override {
            return matrixHeight();
        }

        uint16_t nbLeds() const override {
            return matrixWidth() * matrixHeight();
        }

        uint16_t segmentSize(uint16_t segmentIndex) const override {
            return matrixWidth();
        }

        PolarCoords toPolarCoords(uint16_t pixelIndex) const override {
            uint16_t mWidth = matrixWidth();
            uint16_t mHeight = matrixHeight();

            if (pixelIndex >= nbLeds()) {
                return {u0x16(0), u0x16(0)};
            }

            uint16_t x = pixelIndex % mWidth;
            uint16_t y = pixelIndex / mWidth;

            const int32_t centered_x = (static_cast<int32_t>(x) * 2) - (mWidth - 1);
            const int32_t centered_y =
                    (static_cast<int32_t>(mHeight - 1 - y) * 2) - (mHeight - 1);

            int32_t denom_x = mWidth > 1 ? (mWidth - 1) : 1;
            int32_t denom_y = mHeight > 1 ? (mHeight - 1) : 1;

            // Centre-crop (aspect-preserving cover): share one denominator so
            // both axes carry the same units-per-pixel. The longer axis spans
            // the full [-1, 1] effect extent; the shorter axis maps to a
            // proportionally smaller central band, so the effect fills the
            // panel undistorted and the overflow is cropped off-screen.
            if (centerCrop()) {
                const int32_t common = denom_x > denom_y ? denom_x : denom_y;
                denom_x = common;
                denom_y = common;
            }

            const int32_t x_q0_16 = (centered_x * S0X16_ONE) / denom_x;
            const int32_t y_q0_16 = (centered_y * S0X16_ONE) / denom_y;

            const u0x16 diagonal_scale(DIAGONAL_SCALE_U0X16);
            const int32_t scaled_x = mulI32U0x16Sat(x_q0_16, diagonal_scale);
            const int32_t scaled_y = mulI32U0x16Sat(y_q0_16, diagonal_scale);

            // Convert to UV space [0, 1] then to Polar UV.
            UV cart_uv(
                fl::s16x16::from_raw((scaled_x + S0X16_ONE) >> 1),
                fl::s16x16::from_raw((scaled_y + S0X16_ONE) >> 1)
            );
            UV polar = cartesianToPolarUV(cart_uv);
            return {
                u0x16(static_cast<uint16_t>(polar.u.raw())),
                u0x16(static_cast<uint16_t>(polar.v.raw()))
            };
        }

        RenderPoint toRenderPoint(uint16_t pixelIndex) const override {
            RenderPoint point = makePolarRenderPoint(toPolarCoords(pixelIndex));
            const uint16_t mWidth = matrixWidth();
            const uint16_t mHeight = matrixHeight();
            if (pixelIndex < nbLeds() && mWidth > 0 && mHeight > 0) {
                point.raster.valid = true;
                point.raster.x = pixelIndex % mWidth;
                point.raster.y = pixelIndex / mWidth;
                point.raster.width = mWidth;
                point.raster.height = mHeight;
            }
            return point;
        }
    };
}
#endif //POLARSHADER_MATRIXDISPLAYSPEC_H
