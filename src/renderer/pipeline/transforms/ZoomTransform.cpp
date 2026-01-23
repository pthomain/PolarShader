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
#include <cstring>
#include "../modulators/BoundUtils.h"
#include "renderer/pipeline/utils/MathUtils.h"

namespace PolarShader {
    namespace {
        constexpr UnboundedScalar kZoomMin = UnboundedScalar::fromRaw(Q16_16_ONE >> 4); // 0.0625x
        constexpr UnboundedScalar kZoomMax = UnboundedScalar::fromRaw(Q16_16_ONE); // 1.0x
    }

    struct ZoomTransform::State {
        BoundedScalarSignal scaleSignal;
        UnboundedScalar scaleValue = kZoomMin;

        explicit State(BoundedScalarSignal s) : scaleSignal(std::move(s)) {
        }
    };

    ZoomTransform::ZoomTransform(BoundedScalarSignal scale)
        : state(std::make_shared<State>(std::move(scale))) {
    }

    void ZoomTransform::advanceFrame(TimeMillis timeInMillis) {
        state->scaleValue = unbound(state->scaleSignal(timeInMillis), kZoomMin, kZoomMax);
    }

    CartesianLayer ZoomTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y) {
            RawQ16_16 s_raw = RawQ16_16(state->scaleValue.asRaw());
            // Allow scale == 0 to collapse to origin; negative scales flip axes.
            int64_t sx = static_cast<int64_t>(x) * raw(s_raw);
            int64_t sy = static_cast<int64_t>(y) * raw(s_raw);

            uint32_t wrappedX = static_cast<uint32_t>(sx >> 16);
            uint32_t wrappedY = static_cast<uint32_t>(sy >> 16);

            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));
            return layer(finalX, finalY);
        };
    }
}
