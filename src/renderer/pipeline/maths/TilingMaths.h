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

#ifndef POLAR_SHADER_PIPELINE_MATHS_TILINGMATHS_H
#define POLAR_SHADER_PIPELINE_MATHS_TILINGMATHS_H

#include <cstdint>

namespace PolarShader::TilingMaths {
    enum class TileShape {
        SQUARE,
        TRIANGLE,
        HEXAGON
    };

    constexpr int32_t MIN_CELL_SIZE_RAW = 16;
    constexpr int32_t MAX_CELL_SIZE_RAW = 256;

    struct TileSample {
        int32_t local_x;
        int32_t local_y;
        int32_t cell_id;
        int32_t cell_a;
        int32_t cell_b;
        int32_t cell_c;
    };

    int32_t normalizeCellSizeRaw(int64_t value, int32_t fallback = MIN_CELL_SIZE_RAW);

    int32_t floorDivide(int32_t value, int32_t divisor);

    TileSample sampleTile(int32_t x_raw, int32_t y_raw, int32_t cell_size_raw, TileShape shape);

    void applyMirror(TileSample &sample, bool mirrored);
}

#endif // POLAR_SHADER_PIPELINE_MATHS_TILINGMATHS_H
