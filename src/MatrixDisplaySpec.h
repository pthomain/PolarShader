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

        uint16_t matrixWidth() const { return displayWidth() / subsample(); }
        uint16_t matrixHeight() const { return displayHeight() / subsample(); }

        // Scale so the unit circle has a diameter equal to the matrix diagonal.
        // 1 / sqrt(2) in f16/sf16 (~0.7071).
        static constexpr uint16_t DIAGONAL_SCALE_F16 = 46341u;

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
                return {0, 0};
            }

            uint16_t x = pixelIndex % mWidth;
            uint16_t y = pixelIndex / mWidth;

            const int32_t centered_x = (static_cast<int32_t>(x) * 2) - (mWidth - 1);
            const int32_t centered_y =
                    (static_cast<int32_t>(mHeight - 1 - y) * 2) - (mHeight - 1);

            const int32_t denom_x = mWidth > 1 ? (mWidth - 1) : 1;
            const int32_t denom_y = mHeight > 1 ? (mHeight - 1) : 1;

            const int32_t x_q0_16 = (centered_x * SF16_ONE) / denom_x;
            const int32_t y_q0_16 = (centered_y * SF16_ONE) / denom_y;

            const f16 diagonal_scale(DIAGONAL_SCALE_F16);
            const int32_t scaled_x = mulI32F16Sat(x_q0_16, diagonal_scale);
            const int32_t scaled_y = mulI32F16Sat(y_q0_16, diagonal_scale);

            // Convert to UV space [0, 1] then to Polar UV.
            UV cart_uv(
                sr16((scaled_x + SF16_ONE) >> 1),
                sr16((scaled_y + SF16_ONE) >> 1)
            );
            UV polar = cartesianToPolarUV(cart_uv);
            return {
                f16(static_cast<uint16_t>(raw(polar.u))),
                f16(static_cast<uint16_t>(raw(polar.v)))
            };
        }
    };
}
#endif //POLARSHADER_MATRIXDISPLAYSPEC_H
