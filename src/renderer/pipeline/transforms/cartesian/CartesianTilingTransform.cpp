//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#include "CartesianTilingTransform.h"
#include "renderer/pipeline/units/CartesianUnits.h"
#include "renderer/pipeline/units/TimeUnits.h"
#include "renderer/pipeline/ranges/ScalarRange.h"
#include <algorithm>
#include <climits>

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
    struct CartesianTilingTransform::State {
        int32_t cellSizeRaw;
        bool mirrored;
        MappedSignal<int32_t> cellSizeSignal;
        bool hasSignal;
    };

    static constexpr int32_t kCellSizeScale = 10000;

    static int32_t clampCellSize(int64_t value) {
        if (value <= INT32_MIN) return INT32_MIN;
        if (value >= INT32_MAX) return INT32_MAX;
        return static_cast<int32_t>(value);
    }

    CartesianTilingTransform::CartesianTilingTransform(uint32_t cellSizeQ24_8, bool mirrored)
        : state(std::make_shared<State>(State{
            clampCellSize(static_cast<int64_t>(cellSizeQ24_8) * kCellSizeScale),
            mirrored,
            MappedSignal<int32_t>(),
            false
        })) {
    }

    CartesianTilingTransform::CartesianTilingTransform(MappedInputs inputs, bool mirrored)
        : state(std::make_shared<State>(State{
            static_cast<int32_t>(INT32_MAX),
            mirrored,
            std::move(inputs.cellSizeSignal),
            true
        })) {
        if (state->cellSizeSignal) {
            int32_t starting = state->cellSizeSignal(TimeMillis(0)).get();
            if (starting <= 0) starting = 1;
            state->cellSizeRaw = starting;
        }
    }

    CartesianTilingTransform::CartesianTilingTransform(
        SFracQ0_16Signal cellSize,
        int32_t minCellSize,
        int32_t maxCellSize,
        bool mirrored
    ) : CartesianTilingTransform(makeInputs(cellSize, minCellSize, maxCellSize), mirrored) {
    }

    CartesianTilingTransform::MappedInputs CartesianTilingTransform::makeInputs(
        SFracQ0_16Signal cellSize,
        int32_t minCellSize,
        int32_t maxCellSize
    ) {
        ScalarRange range(minCellSize, maxCellSize);
        auto signal = range.mapSignal(std::move(cellSize));
        return MappedInputs{
            [signal = std::move(signal)](TimeMillis time) mutable {
                int32_t value = signal(time).get();
                int64_t scaled = static_cast<int64_t>(value) * kCellSizeScale;
                return MappedValue<int32_t>(clampCellSize(scaled));
            }
        };
    }

    void CartesianTilingTransform::advanceFrame(TimeMillis timeInMillis) {
        if (state->hasSignal && state->cellSizeSignal) {
            int32_t value = state->cellSizeSignal(timeInMillis).get();
            if (value <= 0) value = 1;
            state->cellSizeRaw = value;
        }
    }

    CartesianLayer CartesianTilingTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](CartQ24_8 x, CartQ24_8 y) {
            int32_t cellSizeRaw = state->cellSizeRaw;
            if (cellSizeRaw <= 0) {
                return layer(x, y);
            }

            int32_t x_raw = raw(x);
            int32_t y_raw = raw(y);
            int32_t col = floorDivide(x_raw, cellSizeRaw);
            int32_t row = floorDivide(y_raw, cellSizeRaw);
            int32_t local_x = x_raw - (col * cellSizeRaw);
            int32_t local_y = y_raw - (row * cellSizeRaw);

            if (state->mirrored && ((col + row) & 1)) {
                local_x = (cellSizeRaw - 1) - local_x;
                local_y = (cellSizeRaw - 1) - local_y;
            }

            return layer(CartQ24_8(local_x), CartQ24_8(local_y));
        };
    }
}
