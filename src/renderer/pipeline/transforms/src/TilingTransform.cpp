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

#include "renderer/pipeline/transforms/TilingTransform.h"
#include "renderer/pipeline/maths/TilingMaths.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/signals/TilingSignalUtils.h"
#include "renderer/pipeline/maths/CartesianMaths.h"

namespace PolarShader {
    struct TilingTransform::State {
        int32_t cellSizeRaw;
        bool mirrored;
        TileShape shape;
        Sf16Signal cellSizeSignal;
        bool hasSignal;
    };

    TilingTransform::TilingTransform(uint32_t cellSizeQ24_8, bool mirrored, TileShape shape)
        : state(std::make_shared<State>(State{
            TilingMaths::normalizeCellSizeRaw(static_cast<int64_t>(cellSizeQ24_8), TilingMaths::MIN_CELL_SIZE_RAW),
            mirrored,
            shape,
            Sf16Signal(),
            false
        })) {
    }

    TilingTransform::TilingTransform(
        Sf16Signal cellSize,
        bool mirrored,
        TileShape shape
    ) : state(std::make_shared<State>(State{
            sampleTilingCellSizeRaw(cellSize, 0, TilingMaths::MAX_CELL_SIZE_RAW),
            mirrored,
            shape,
            std::move(cellSize),
            true
        })) {}

    void TilingTransform::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void)progress;
        if (state->hasSignal && state->cellSizeSignal) {
            state->cellSizeRaw = sampleTilingCellSizeRaw(
                state->cellSizeSignal,
                elapsedMs,
                state->cellSizeRaw
            );
        }
    }

    int32_t TilingTransform::getCellSizeRaw() const {
        return state->cellSizeRaw;
    }

    UVMap TilingTransform::operator()(const UVMap &layer) const {
        return [state = this->state, layer](UV uv) {
            int32_t cellSizeRaw = state->cellSizeRaw;
            if (cellSizeRaw <= 0) {
                return layer(uv);
            }

            sr8 cx = CartesianMaths::from_uv(uv.u);
            sr8 cy = CartesianMaths::from_uv(uv.v);
            int32_t x_raw = raw(cx);
            int32_t y_raw = raw(cy);
            TilingMaths::TileSample tile = TilingMaths::sampleTile(x_raw, y_raw, cellSizeRaw, state->shape);
            TilingMaths::applyMirror(tile, state->mirrored);

            UV tiled_uv(
                CartesianMaths::to_uv(sr8(tile.local_x)),
                CartesianMaths::to_uv(sr8(tile.local_y))
            );

            return layer(tiled_uv);
        };
    }
}
