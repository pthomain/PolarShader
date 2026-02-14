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

#ifndef POLARSHADER_MATRIX128X128DISPLAYSPEC_H
#define POLARSHADER_MATRIX128X128DISPLAYSPEC_H

#include "MatrixDisplaySpec.h"

namespace PolarShader {
    class Matrix128x128DisplaySpec : public MatrixDisplaySpec {
    public:
        static constexpr uint16_t PANEL_WIDTH = 64;
        static constexpr uint16_t PANEL_HEIGHT = 64;
        static constexpr uint16_t DISPLAY_WIDTH = PANEL_WIDTH * 2;
        static constexpr uint16_t DISPLAY_HEIGHT = PANEL_HEIGHT * 2;
        static constexpr uint16_t SUBSAMPLE = 2;

        uint16_t displayWidth() const override { return DISPLAY_WIDTH; }
        uint16_t displayHeight() const override { return DISPLAY_HEIGHT; }
        uint16_t subsample() const override { return SUBSAMPLE; }
    };
}
#endif //POLARSHADER_MATRIX128X128DISPLAYSPEC_H
