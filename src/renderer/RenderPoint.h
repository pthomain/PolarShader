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

#ifndef POLAR_SHADER_RENDER_POINT_H
#define POLAR_SHADER_RENDER_POINT_H

#include "renderer/pipeline/maths/units/Units.h"

namespace PolarShader {
    using PolarCoords = fl::pair<u0x16, u0x16>;

    struct RasterDisplayInfo {
        bool valid{false};
        uint16_t width{0};
        uint16_t height{0};
        uint32_t cellCount{0};
    };

    struct RasterPoint {
        bool valid{false};
        uint16_t x{0};
        uint16_t y{0};
        uint16_t width{0};
        uint16_t height{0};
    };

    struct RenderPoint {
        u0x16 angle{0};
        u0x16 radius{0};
        RasterPoint raster{};
    };

    inline RenderPoint makePolarRenderPoint(PolarCoords coords) {
        return RenderPoint{coords.first, coords.second, RasterPoint{}};
    }
}

#endif // POLAR_SHADER_RENDER_POINT_H
