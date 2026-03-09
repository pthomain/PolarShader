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

#include "renderer/pipeline/patterns/TilingPattern.h"
#include "renderer/pipeline/maths/CartesianMaths.h"
#include "renderer/pipeline/maths/TilingMaths.h"
#include "renderer/pipeline/signals/TilingSignalUtils.h"
#include <utility>

namespace PolarShader {
    namespace {
        uint8_t modPositive(int32_t value, uint8_t mod) {
            int32_t r = value % mod;
            if (r < 0) r += mod;
            return static_cast<uint8_t>(r);
        }

        uint16_t mapColorValue(uint8_t index, uint8_t colors) {
            if (colors <= 1) return SF16_MAX;
            uint32_t denominator = static_cast<uint32_t>(colors > 1 ? colors - 1 : colors);
            if (denominator == 0) return SF16_MAX;
            uint32_t numerator = static_cast<uint32_t>(index) * SF16_MAX;
            uint16_t value = static_cast<uint16_t>(numerator / denominator);
            return value;
        }

        int32_t hexRoundF16(int32_t v_f16) {
            return (v_f16 >= 0) ? (v_f16 + 32768) >> 16 : -(((-v_f16) + 32767) >> 16);
        }

        int32_t abs32(int32_t value) {
            return (value < 0) ? -value : value;
        }

        uint8_t colorIndexForCell(TilingMaths::TileShape shape, int32_t a, int32_t b, int32_t c, uint8_t colors) {
            switch (shape) {
                case TilingMaths::TileShape::SQUARE:
                    return modPositive(a + b, colors);
                case TilingMaths::TileShape::TRIANGLE:
                    return modPositive(a - (2 * b) - c, colors);
                case TilingMaths::TileShape::HEXAGON:
                default:
                    return modPositive(a - b, colors);
            }
        }

    }

    struct TilingPattern::UVTilingFunctor {
        const State *state;
        uint8_t color_count;
        TileShape shape;

        PatternNormU16 operator()(UV uv) const {
            int32_t cell_size_raw = state ? state->cell_size_raw : 0;
            if (cell_size_raw <= 0) return PatternNormU16(0);

            uint8_t colors = (color_count < 3) ? 3 : color_count;

            int32_t x_raw = CartesianMaths::from_uv(uv.u).raw();
            int32_t y_raw = CartesianMaths::from_uv(uv.v).raw();

            if (shape == TileShape::SQUARE) {
                TilingMaths::TileSample tile = TilingMaths::sampleTile(x_raw, y_raw, cell_size_raw, shape);
                return PatternNormU16(
                    mapColorValue(colorIndexForCell(shape, tile.cell_a, tile.cell_b, tile.cell_c, colors), colors)
                );
            }

            if (shape == TileShape::TRIANGLE) {
                TilingMaths::TileSample tile = TilingMaths::sampleTile(x_raw, y_raw, cell_size_raw, shape);
                return PatternNormU16(
                    mapColorValue(colorIndexForCell(shape, tile.cell_a, tile.cell_b, tile.cell_c, colors), colors)
                );
            }

            // Flat-top axial conversion (Q16.16)
            int64_t q_num = static_cast<int64_t>(x_raw) * 43691;
            int64_t r_num = static_cast<int64_t>(y_raw) * 37837 - static_cast<int64_t>(x_raw) * 21845;
            
            int32_t q_f16 = static_cast<int32_t>(q_num / cell_size_raw);
            int32_t r_f16 = static_cast<int32_t>(r_num / cell_size_raw);
            int32_t s_f16 = -q_f16 - r_f16;

            // Find primary center
            int32_t rx = hexRoundF16(q_f16);
            int32_t rz = hexRoundF16(r_f16);
            int32_t ry = hexRoundF16(s_f16);

            int32_t dq0 = q_f16 - (rx << 16);
            int32_t dr0 = r_f16 - (rz << 16);
            int32_t ds0 = s_f16 - (ry << 16);

            if (abs32(dq0) > abs32(dr0) && abs32(dq0) > abs32(ds0)) {
                rx = -ry - rz;
            } else if (abs32(dr0) > abs32(ds0)) {
                rz = -rx - ry;
            }
            
            return PatternNormU16(
                mapColorValue(colorIndexForCell(shape, rx, rz, 0, colors), colors)
            );
        }
    };

    TilingPattern::TilingPattern(
        uint16_t cellSize,
        uint8_t colorCount,
        TileShape shape
    )
        : cell_size_u16(cellSize == 0 ? 32 : cellSize),
          color_count(colorCount < 3 ? 3 : colorCount),
          shape(shape) {
        state.cell_size_raw = cell_size_u16;
    }

    TilingPattern::TilingPattern(
        Sf16Signal cellSize,
        uint8_t colorCount,
        TileShape shape
    ) : cell_size_signal(std::move(cellSize)),
        cell_size_u16(0),
        color_count(colorCount < 3 ? 3 : colorCount),
        shape(shape) {
    }

    void TilingPattern::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void)progress;
        if (cell_size_signal) {
            state.cell_size_raw = sampleTilingCellSizeRaw(
                cell_size_signal,
                elapsedMs,
                TilingMaths::MIN_CELL_SIZE_RAW
            );
        } else {
            state.cell_size_raw = TilingMaths::normalizeCellSizeRaw(
                cell_size_u16,
                TilingMaths::MIN_CELL_SIZE_RAW
            );
        }
    }

    UVMap TilingPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        (void)context;
        return UVTilingFunctor{&state, color_count, shape};
    }
}
