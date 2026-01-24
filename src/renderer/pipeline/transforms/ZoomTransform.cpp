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

#include "ZoomTransform.h"
#include "renderer/pipeline/maths/Maths.h"

namespace PolarShader {
    namespace {
        constexpr ScalarQ0_16 kZoomMin = ScalarQ0_16(Q0_16_ONE >> 4); // 0.0625x
        constexpr ScalarQ0_16 kZoomMax = ScalarQ0_16(Q0_16_MAX); // ~1.0x
    }

    struct ZoomTransform::State {
        FracQ0_16Signal scaleSignal;
        ScalarQ0_16 scaleValue = kZoomMin;

        explicit State(FracQ0_16Signal s) : scaleSignal(std::move(s)) {
        }
    };

    ZoomTransform::ZoomTransform(FracQ0_16Signal scale)
        : state(std::make_shared<State>(std::move(scale))) {
    }

    void ZoomTransform::advanceFrame(TimeMillis timeInMillis) {
        state->scaleValue = toScalar(state->scaleSignal(timeInMillis), kZoomMin, kZoomMax);
    }

    CartesianLayer ZoomTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y) {
            // Allow scale == 0 to collapse to origin; negative scales flip axes.
            int64_t sx = static_cast<int64_t>(x) * static_cast<int64_t>(raw(state->scaleValue));
            int64_t sy = static_cast<int64_t>(y) * static_cast<int64_t>(raw(state->scaleValue));

            int32_t finalX = static_cast<int32_t>(sx >> 16);
            int32_t finalY = static_cast<int32_t>(sy >> 16);
            return layer(finalX, finalY);
        };
    }
}
