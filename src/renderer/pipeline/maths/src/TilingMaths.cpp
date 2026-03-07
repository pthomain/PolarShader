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

#include "renderer/pipeline/maths/TilingMaths.h"
#include <climits>

namespace PolarShader::TilingMaths {
    int32_t normalizeCellSizeRaw(int64_t value, int32_t fallback) {
        if (value < INT32_MIN) value = INT32_MIN;
        if (value > INT32_MAX) value = INT32_MAX;

        int32_t normalized = static_cast<int32_t>(value);
        if (normalized <= 0) {
            return fallback <= 0 ? 1 : fallback;
        }
        return normalized;
    }

    int32_t floorDivide(int32_t value, int32_t divisor) {
        if (divisor == 0) return 0;
        if (value >= 0) return value / divisor;
        int32_t div = value / divisor;
        int32_t rem = value % divisor;
        if (rem != 0) --div;
        return div;
    }

    TileSample sampleTile(int32_t x_raw, int32_t y_raw, int32_t cell_size_raw, TileShape shape) {
        TileSample sample{
            x_raw, y_raw, 0, 0, 0, 0
        };

        if (cell_size_raw <= 0) {
            return sample;
        }

        if (shape == TileShape::SQUARE) {
            int32_t col = floorDivide(x_raw, cell_size_raw);
            int32_t row = floorDivide(y_raw, cell_size_raw);
            sample.local_x = x_raw - (col * cell_size_raw);
            sample.local_y = y_raw - (row * cell_size_raw);
            sample.cell_id = col ^ row;
            sample.cell_a = col;
            sample.cell_b = row;
            sample.cell_c = 0;
            return sample;
        }

        if (shape == TileShape::TRIANGLE) {
            int32_t h = (static_cast<int64_t>(cell_size_raw) * 222) >> 8;
            if (h <= 0) h = 1;

            int32_t row = floorDivide(y_raw, h);
            int32_t x_offset = (row & 1) ? (cell_size_raw / 2) : 0;
            int32_t col = floorDivide(x_raw - x_offset, cell_size_raw);

            int32_t rel_x = x_raw - (col * cell_size_raw + x_offset);
            int32_t rel_y = y_raw - (row * h);
            int32_t dx = rel_x - (cell_size_raw / 2);
            if (dx < 0) dx = -dx;

            bool inside_up = rel_y > ((static_cast<int64_t>(dx) * 443) >> 8);

            if (inside_up) {
                int32_t center_x = col * cell_size_raw + x_offset + (cell_size_raw / 2);
                int32_t center_y = row * h + (h * 2 / 3);
                sample.local_x = x_raw - center_x;
                sample.local_y = y_raw - center_y;
                sample.cell_id = col ^ row ^ 0x55555555;
                sample.cell_a = col;
                sample.cell_b = row;
                sample.cell_c = 1;
            } else {
                int32_t tri_col = (rel_x < cell_size_raw / 2) ? col : (col + 1);
                int32_t center_x = tri_col * cell_size_raw + x_offset;
                int32_t center_y = row * h + (h / 3);
                sample.local_x = center_x - x_raw;
                sample.local_y = center_y - y_raw;
                sample.cell_id = tri_col ^ row ^ 0xAAAAAAAA;
                sample.cell_a = tri_col;
                sample.cell_b = row;
                sample.cell_c = 0;
            }
            return sample;
        }

        int32_t w = (static_cast<int64_t>(cell_size_raw) * 443) >> 8;
        int32_t h = (static_cast<int64_t>(cell_size_raw) * 3) >> 1;
        if (w <= 0) w = 1;
        if (h <= 0) h = 1;

        int32_t row = floorDivide(y_raw, h);
        bool offset_row = (row & 1) != 0;
        int32_t x_offset = offset_row ? (w / 2) : 0;
        int32_t col = floorDivide(x_raw - x_offset, w);

        int32_t rel_x = x_raw - (col * w + x_offset);
        int32_t rel_y = y_raw - (row * h);

        int32_t s2 = cell_size_raw / 2;
        if (rel_y < s2) {
            int32_t left_boundary = s2 - ((static_cast<int64_t>(rel_x) * 148) >> 8);
            int32_t right_boundary = s2 - ((static_cast<int64_t>(w - rel_x) * 148) >> 8);

            if (rel_x < w / 2) {
                if (rel_y < left_boundary) {
                    row--;
                    if (!offset_row) col--;
                }
            } else if (rel_y < right_boundary) {
                row--;
                if (offset_row) col++;
            }
        }

        int32_t center_y = row * h + cell_size_raw;
        int32_t center_x = col * w + ((row & 1) ? w : w / 2);

        sample.local_x = x_raw - center_x;
        sample.local_y = y_raw - center_y;
        sample.cell_id = col ^ row;
        sample.cell_a = col;
        sample.cell_b = row;
        sample.cell_c = 0;
        return sample;
    }

    void applyMirror(TileSample &sample, bool mirrored) {
        if (!mirrored || ((sample.cell_id & 1) == 0)) {
            return;
        }
        sample.local_x = -sample.local_x;
        sample.local_y = -sample.local_y;
    }
}
