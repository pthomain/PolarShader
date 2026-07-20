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

#include "renderer/pipeline/patterns/XORPattern.h"
#include "renderer/pipeline/patterns/GridUtils.h"

namespace PolarShader {
    namespace {
        uint8_t xorCellForAxis(fl::s16x16 axis, uint8_t gridSize) {
            int32_t wrappedRaw = raw(axis) % static_cast<int32_t>(ANGLE_FULL_TURN_U32);
            if (wrappedRaw < 0) wrappedRaw += static_cast<int32_t>(ANGLE_FULL_TURN_U32);
            return static_cast<uint8_t>((static_cast<uint32_t>(wrappedRaw) * gridSize) >> 16);
        }
    }

    uint8_t XORPattern::clampGridSize(uint8_t value) {
        if (value < grid::kMinGridSize) return grid::kMinGridSize;
        if (value > grid::kMaxGridSize) return grid::kMaxGridSize;
        return value;
    }

    struct XORPattern::Functor {
        const State *state;

        PatternNormU16 operator()(UV uv) const {
            const uint8_t x = xorCellForAxis(uv.u, state->gridSize);
            const uint8_t y = xorCellForAxis(uv.v, state->gridSize);
            const uint16_t xr = static_cast<uint16_t>(x ^ y);
            const uint16_t ramp = static_cast<uint16_t>(
                (static_cast<uint32_t>(xr) * U0X16_MAX) / (state->gridSize - 1u)
            );
            return PatternNormU16(static_cast<uint16_t>(ramp + state->phase));
        }
    };

    XORPattern::XORPattern(uint8_t gridSize, uint16_t speed)
        : state(std::make_shared<State>()) {
        state->gridSize = clampGridSize(gridSize);
        state->speed = speed;
    }

    void XORPattern::advanceFrame(u0x16 progress, TimeMillis elapsedMs) {
        (void) progress;
        state->phase = static_cast<uint16_t>(
            (static_cast<uint32_t>(elapsedMs) * state->speed) >> 6
        );
    }

    UVMap XORPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        (void) context;
        return Functor{state.get()};
    }
}
