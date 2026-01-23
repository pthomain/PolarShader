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

#include "ShearTransform.h"
#include <cstring>
#include "../modulators/BoundUtils.h"
#include "renderer/pipeline/utils/MathUtils.h"

namespace PolarShader {
    namespace {
        constexpr UnboundedScalar kShearMin = UnboundedScalar::fromRaw(-static_cast<int32_t>(Q16_16_ONE / 4));
        constexpr UnboundedScalar kShearMax = UnboundedScalar::fromRaw(Q16_16_ONE / 4);
    }

    struct ShearTransform::State {
        BoundedScalarSignal kxSignal;
        BoundedScalarSignal kySignal;
        UnboundedScalar kxValue = kShearMin;
        UnboundedScalar kyValue = kShearMin;

        State(BoundedScalarSignal kx, BoundedScalarSignal ky)
            : kxSignal(std::move(kx)), kySignal(std::move(ky)) {
        }
    };

    ShearTransform::ShearTransform(BoundedScalarSignal kx, BoundedScalarSignal ky)
        : state(std::make_shared<State>(std::move(kx), std::move(ky))) {
    }

    void ShearTransform::advanceFrame(TimeMillis timeInMillis) {
        state->kxValue = unbound(state->kxSignal(timeInMillis), kShearMin, kShearMax);
        state->kyValue = unbound(state->kySignal(timeInMillis), kShearMin, kShearMax);
    }

    CartesianLayer ShearTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y) {
            RawQ16_16 kx_raw = RawQ16_16(state->kxValue.asRaw());
            RawQ16_16 ky_raw = RawQ16_16(state->kyValue.asRaw());

            int64_t dx = static_cast<int64_t>(y) * raw(kx_raw);
            int64_t dy = static_cast<int64_t>(x) * raw(ky_raw);

            uint32_t wrappedX = static_cast<uint32_t>(static_cast<int64_t>(x) + (dx >> 16));
            uint32_t wrappedY = static_cast<uint32_t>(static_cast<int64_t>(y) + (dy >> 16));

            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));

            return layer(finalX, finalY);
        };
    }
}
