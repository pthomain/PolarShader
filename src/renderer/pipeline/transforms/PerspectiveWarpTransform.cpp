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

#include "PerspectiveWarpTransform.h"
#include <cstring>
#include "../modulators/BoundUtils.h"
#include "renderer/pipeline/utils/MathUtils.h"

namespace PolarShader {
    namespace {
        constexpr UnboundedScalar kWarpMin = UnboundedScalar::fromRaw(-static_cast<int32_t>(Q16_16_ONE / 4));
        constexpr UnboundedScalar kWarpMax = UnboundedScalar::fromRaw(Q16_16_ONE / 4);
    }

    struct PerspectiveWarpTransform::State {
        BoundedScalarSignal kSignal;
        UnboundedScalar kValue = kWarpMin;

        explicit State(BoundedScalarSignal k) : kSignal(std::move(k)) {
        }
    };

    PerspectiveWarpTransform::PerspectiveWarpTransform(BoundedScalarSignal k)
        : state(std::make_shared<State>(std::move(k))) {
    }

    void PerspectiveWarpTransform::advanceFrame(TimeMillis timeInMillis) {
        state->kValue = unbound(state->kSignal(timeInMillis), kWarpMin, kWarpMax);
    }

    CartesianLayer PerspectiveWarpTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y) {
            RawQ16_16 k_raw = RawQ16_16(state->kValue.asRaw());
            int64_t ky = (static_cast<int64_t>(raw(k_raw)) * y) >> 16;
            int64_t denom = static_cast<int64_t>(Q16_16_ONE) + ky;

            // Clamp denom to avoid singularity and overflow.
            // Ensure minimum magnitude to prevent division by zero or explosive scaling.
            // 64 in Q16.16 is ~0.001.
            if (denom > -64 && denom < 64) {
                denom = (denom >= 0) ? 64 : -64;
            }

            int64_t scale_q16 = (static_cast<int64_t>(Q16_16_ONE) << 16) / denom; // Q16.16 factor

            int64_t sx = (static_cast<int64_t>(x) * scale_q16 + (1LL << 15)) >> 16;
            int64_t sy = (static_cast<int64_t>(y) * scale_q16 + (1LL << 15)) >> 16;

            uint32_t wrappedX = static_cast<uint32_t>(sx);
            uint32_t wrappedY = static_cast<uint32_t>(sy);

            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));

            return layer(finalX, finalY);
        };
    }
}
