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

#include "renderer/pipeline/transforms/CartesianTilingTransform.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/maths/CartesianMaths.h"
#include <algorithm>

namespace {
    int32_t floorDivide(int32_t value, int32_t divisor) {
        if (divisor == 0) return 0;
        if (value >= 0) return value / divisor;
        int32_t div = value / divisor;
        int32_t rem = value % divisor;
        if (rem != 0) --div;
        return div;
    }
}

namespace PolarShader {
    namespace {
        // Default Tiling boundaries in sr8 (Q24.8)
        const int32_t MIN_CELL_SIZE_RAW = 16; // ~0.06x of full sr8 range
        const int32_t MAX_CELL_SIZE_RAW = 256; // 1.0x of full sr8 range
    }

    static int32_t clampCellSize(int64_t value) {
        if (value <= INT32_MIN) return INT32_MIN;
        if (value >= INT32_MAX) return INT32_MAX;
        return static_cast<int32_t>(value);
    }

    struct CartesianTilingTransform::MappedInputs {
        Sf16Signal cellSizeSignal;
        MagnitudeRange<int32_t> cellSizeRange;
    };

    CartesianTilingTransform::MappedInputs CartesianTilingTransform::makeInputs(
        Sf16Signal cellSize
    ) {
        return MappedInputs{
            std::move(cellSize),
            MagnitudeRange(MIN_CELL_SIZE_RAW, MAX_CELL_SIZE_RAW)
        };
    }

    struct CartesianTilingTransform::State {
        int32_t cellSizeRaw;
        bool mirrored;
        TileShape shape;
        Sf16Signal cellSizeSignal;
        MagnitudeRange<int32_t> cellSizeRange;
        bool hasSignal;
    };

    CartesianTilingTransform::CartesianTilingTransform(uint32_t cellSizeQ24_8, bool mirrored, TileShape shape)
        : state(std::make_shared<State>(State{
            clampCellSize(cellSizeQ24_8),
            mirrored,
            shape,
            Sf16Signal(),
            MagnitudeRange<int32_t>(1, 1),
            false
        })) {
    }

    CartesianTilingTransform::CartesianTilingTransform(
        Sf16Signal cellSize,
        bool mirrored,
        TileShape shape
    ) {
        auto inputs = makeInputs(std::move(cellSize));
        state = std::make_shared<State>(State{
            MAX_CELL_SIZE_RAW,
            mirrored,
            shape,
            std::move(inputs.cellSizeSignal),
            std::move(inputs.cellSizeRange),
            true
        });
        if (state->cellSizeSignal) {
            int32_t starting = state->cellSizeSignal.sample(state->cellSizeRange, 0);
            starting = clampCellSize(starting);
            if (starting <= 0) starting = 1;
            state->cellSizeRaw = starting;
        }
    }

    void CartesianTilingTransform::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        if (state->hasSignal && state->cellSizeSignal) {
            int32_t value = state->cellSizeSignal.sample(state->cellSizeRange, elapsedMs);
            value = clampCellSize(value);
            if (value <= 0) value = 1;
            state->cellSizeRaw = value;
        }
    }

    int32_t CartesianTilingTransform::getCellSizeRaw() const {
        return state->cellSizeRaw;
    }

    UVMap CartesianTilingTransform::operator()(const UVMap &layer) const {
        return [state = this->state, layer](UV uv) {
            int32_t cellSizeRaw = state->cellSizeRaw;
            if (cellSizeRaw <= 0) {
                return layer(uv);
            }

            sr8 cx = CartesianMaths::from_uv(uv.u);
            sr8 cy = CartesianMaths::from_uv(uv.v);
            int32_t x_raw = raw(cx);
            int32_t y_raw = raw(cy);

            int32_t local_x, local_y;

            if (state->shape == TileShape::SQUARE) {
                int32_t col = floorDivide(x_raw, cellSizeRaw);
                int32_t row = floorDivide(y_raw, cellSizeRaw);
                local_x = x_raw - (col * cellSizeRaw);
                local_y = y_raw - (row * cellSizeRaw);

                if (state->mirrored && ((col + row) & 1)) {
                    local_x = (cellSizeRaw - 1) - local_x;
                    local_y = (cellSizeRaw - 1) - local_y;
                }
            } else if (state->shape == TileShape::TRIANGLE) {
                // Correct equilateral triangle tiling.
                // side = cellSizeRaw.
                // Height h = side * sqrt(3)/2. 
                // sqrt(3)/2 approx 0.866. In Q8.8 it is 222.
                int32_t h = (static_cast<int64_t>(cellSizeRaw) * 222) >> 8;
                if (h <= 0) h = 1;

                // We use a rectangular grid of width 'side' and height 'h'.
                // Each rectangle contains two triangles (one 'up', one 'down' or split).
                // A better approach for equilateral triangles:
                // Shift every other row by side/2.
                int32_t row = floorDivide(y_raw, h);
                int32_t x_offset = (row & 1) ? (cellSizeRaw / 2) : 0;
                int32_t col = floorDivide(x_raw - x_offset, cellSizeRaw);

                int32_t rel_x = x_raw - (col * cellSizeRaw + x_offset);
                int32_t rel_y = y_raw - (row * h);

                // Now we are in a rectangle of (cellSizeRaw x h).
                // The rectangle contains one "Up" triangle in the middle and two halves of "Down" triangles on the sides.
                // Boundary: y < h - (2*h/side) * |x - side/2|
                // h / (side/2) = 2h / side = sqrt(3).
                // sqrt(3) approx 1.732. In Q8.8 it is 443.

                int32_t dx = rel_x - (cellSizeRaw / 2);
                if (dx < 0) dx = -dx;

                int32_t boundary_y = h - ((static_cast<int64_t>(dx) * 443) >> 8);
                bool is_up = rel_y > (h - boundary_y);
                // Wait, if rel_y is small (top of rect), it's the peak of the triangle.
                // Actually:
                // Up triangle: (0, h), (side, h), (side/2, 0)
                // Boundary is: rel_y > (2*h/side) * |rel_x - side/2|
                bool inside_up = rel_y > ((static_cast<int64_t>(dx) * 443) >> 8);

                int32_t center_x, center_y;
                int32_t cell_id;

                if (inside_up) {
                    center_x = col * cellSizeRaw + x_offset + (cellSizeRaw / 2);
                    center_y = row * h + (h * 2 / 3); // Centroid
                    cell_id = col ^ row ^ 0x55555555;
                    local_x = x_raw - center_x;
                    local_y = y_raw - center_y;
                } else {
                    // It's in one of the "down" triangles.
                    // If rel_x < side/2, it's the down triangle to the left.
                    // If rel_x > side/2, it's the down triangle to the right.
                    if (rel_x < cellSizeRaw / 2) {
                        center_x = col * cellSizeRaw + x_offset;
                        center_y = row * h + (h / 3);
                        cell_id = col ^ row ^ 0xAAAAAAAA;
                    } else {
                        center_x = (col + 1) * cellSizeRaw + x_offset;
                        center_y = row * h + (h / 3);
                        cell_id = (col + 1) ^ row ^ 0xAAAAAAAA;
                    }
                    local_x = center_x - x_raw; // Invert for down triangles
                    local_y = center_y - y_raw;
                }

                if (state->mirrored && (cell_id & 1)) {
                    local_x = -local_x;
                    local_y = -local_y;
                }
            } else if (state->shape == TileShape::HEXAGON) {
                // Pointy-top hexagon tiling.
                // side = cellSizeRaw.
                // Width w = side * sqrt(3).
                // Height h = side * 3/2.
                // sqrt(3) is approx 1.732. In Q8.8 it's 443.
                int32_t w = (static_cast<int64_t>(cellSizeRaw) * 443) >> 8;
                int32_t h = (static_cast<int64_t>(cellSizeRaw) * 3) >> 1;
                if (w <= 0) w = 1;
                if (h <= 0) h = 1;

                // Spacing:
                // Horizontal spacing = w
                // Vertical spacing = h (which is 1.5 * side)
                // Every other row is offset by w/2.

                int32_t row = floorDivide(y_raw, h);
                bool offset_row = (row & 1) != 0;
                int32_t x_offset = offset_row ? (w / 2) : 0;
                int32_t col = floorDivide(x_raw - x_offset, w);

                int32_t rel_x = x_raw - (col * w + x_offset);
                int32_t rel_y = y_raw - (row * h);

                // We are in a rectangle of size w by h.
                // The top part of the rectangle (y < side/2) might belong to the hexes above.
                int32_t s2 = cellSizeRaw / 2;
                if (rel_y < s2) {
                    // Corner check.
                    // The hex boundary is a diagonal from (0, s2) to (w/2, 0) and (w/2, 0) to (w, s2).
                    // side / w = 1 / sqrt(3).
                    // sqrt(3) is 443 in Q8.8. 1/sqrt(3) is approx 0.577. In Q8.8 it is 148.

                    int32_t left_boundary = s2 - ((static_cast<int64_t>(rel_x) * 148) >> 8);
                    int32_t right_boundary = s2 - ((static_cast<int64_t>(w - rel_x) * 148) >> 8);

                    if (rel_x < w / 2) {
                        if (rel_y < left_boundary) {
                            // Upper-left hex
                            row--;
                            if (!offset_row) col--;
                        }
                    } else {
                        if (rel_y < right_boundary) {
                            // Upper-right hex
                            row--;
                            if (offset_row) col++;
                        }
                    }
                }

                // Recalculate cell center for local UV
                int32_t center_y = row * h + cellSizeRaw;
                int32_t center_x = col * w + ((row & 1) ? w : w / 2);

                local_x = x_raw - center_x;
                local_y = y_raw - center_y;

                if (state->mirrored) {
                    // Stable cell ID for mirroring
                    int32_t cell_id = (col ^ row);
                    if (cell_id & 1) {
                        local_x = -local_x;
                        local_y = -local_y;
                    }
                }
            } else {
                local_x = x_raw;
                local_y = y_raw;
            }

            UV tiled_uv(
                CartesianMaths::to_uv(sr8(local_x)),
                CartesianMaths::to_uv(sr8(local_y))
            );

            return layer(tiled_uv);
        };
    }
}
