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
    static constexpr int32_t kCellSizeScale = 10000;

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
        Sf16Signal cellSize,
        int32_t minCellSize,
        int32_t maxCellSize
    ) {
        return MappedInputs{
            std::move(cellSize),
            MagnitudeRange(minCellSize, maxCellSize)
        };
    }

    struct CartesianTilingTransform::State {
        int32_t cellSizeRaw;
        bool mirrored;
        Sf16Signal cellSizeSignal;
        MagnitudeRange<int32_t> cellSizeRange;
        bool hasSignal;
    };

    CartesianTilingTransform::CartesianTilingTransform(uint32_t cellSizeQ24_8, bool mirrored)
        : state(std::make_shared<State>(State{
            clampCellSize(static_cast<int64_t>(cellSizeQ24_8) * kCellSizeScale),
            mirrored,
            Sf16Signal(),
            MagnitudeRange<int32_t>(1, 1),
            false
        })) {
    }

    CartesianTilingTransform::CartesianTilingTransform(
        Sf16Signal cellSize,
        int32_t minCellSize,
        int32_t maxCellSize,
        bool mirrored
    ) {
        auto inputs = makeInputs(std::move(cellSize), minCellSize, maxCellSize);
        state = std::make_shared<State>(State{
            INT32_MAX,
            mirrored,
            std::move(inputs.cellSizeSignal),
            std::move(inputs.cellSizeRange),
            true
        });
        if (state->cellSizeSignal) {
            int32_t starting = state->cellSizeSignal.sample(state->cellSizeRange, 0);
            starting = clampCellSize(static_cast<int64_t>(starting) * kCellSizeScale);
            if (starting <= 0) starting = 1;
            state->cellSizeRaw = starting;
        }
    }

    void CartesianTilingTransform::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        if (state->hasSignal && state->cellSizeSignal) {
            int32_t value = state->cellSizeSignal.sample(state->cellSizeRange, elapsedMs);
            value = clampCellSize(static_cast<int64_t>(value) * kCellSizeScale);
            if (value <= 0) value = 1;
            state->cellSizeRaw = value;
        }
    }

    UVMap CartesianTilingTransform::operator()(const UVMap &layer) const {
        return [state = this->state, layer](UV uv) {
            int32_t cellSizeRaw = state->cellSizeRaw;
            if (cellSizeRaw <= 0) {
                return layer(uv);
            }

            // Convert UV to raw sr8/r8 Cartesian logic scale (which is what cellSizeRaw is in)
            // Wait, cellSizeRaw in this class seems to becellSizeQ24_8 * 10000? 
            // That's unusual. Let's check from_uv.
            // from_uv(uv) returns sr8/r8 Cartesian (sr8/r8).
            
            sr8 cx = CartesianMaths::from_uv(uv.u);
            sr8 cy = CartesianMaths::from_uv(uv.v);
            
            // Tiling logic using existing CartesianLayer operator's math
            int32_t x_raw = raw(cx);
            int32_t y_raw = raw(cy);
            int32_t col = floorDivide(x_raw, cellSizeRaw);
            int32_t row = floorDivide(y_raw, cellSizeRaw);
            int32_t local_x = x_raw - (col * cellSizeRaw);
            int32_t local_y = y_raw - (row * cellSizeRaw);

            if (state->mirrored && ((col + row) & 1)) {
                local_x = (cellSizeRaw - 1) - local_x;
                local_y = (cellSizeRaw - 1) - local_y;
            }
            
            UV tiled_uv(
                CartesianMaths::to_uv(sr8(local_x)),
                CartesianMaths::to_uv(sr8(local_y))
            );

            return layer(tiled_uv);
        };
    }
}
