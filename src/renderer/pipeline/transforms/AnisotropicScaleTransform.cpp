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

#include "AnisotropicScaleTransform.h"
#include <cstring>
#include "../modulators/BoundUtils.h"
#include "renderer/pipeline/utils/MathUtils.h"

namespace PolarShader {
    namespace {
        constexpr UnboundedScalar kScaleMin = UnboundedScalar(0);
        constexpr UnboundedScalar kScaleMax = UnboundedScalar(4);
    }

    struct AnisotropicScaleTransform::State {
        BoundedScalarSignal sxSignal;
        BoundedScalarSignal sySignal;
        UnboundedScalar sxValue = kScaleMin;
        UnboundedScalar syValue = kScaleMin;

        State(BoundedScalarSignal sx, BoundedScalarSignal sy)
            : sxSignal(std::move(sx)), sySignal(std::move(sy)) {
        }
    };

    AnisotropicScaleTransform::AnisotropicScaleTransform(BoundedScalarSignal sx,
                                                         BoundedScalarSignal sy)
        : state(std::make_shared<State>(std::move(sx), std::move(sy))) {
    }

    void AnisotropicScaleTransform::advanceFrame(TimeMillis timeInMillis) {
        state->sxValue = unbound(state->sxSignal(timeInMillis), kScaleMin, kScaleMax);
        state->syValue = unbound(state->sySignal(timeInMillis), kScaleMin, kScaleMax);
    }

    CartesianLayer AnisotropicScaleTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y) {
            RawQ16_16 sx_raw = RawQ16_16(state->sxValue.asRaw());
            RawQ16_16 sy_raw = RawQ16_16(state->syValue.asRaw());

            int64_t scaledX = static_cast<int64_t>(x) * raw(sx_raw);
            int64_t scaledY = static_cast<int64_t>(y) * raw(sy_raw);

            uint32_t wrappedX = static_cast<uint32_t>(scaledX >> 16);
            uint32_t wrappedY = static_cast<uint32_t>(scaledY >> 16);

            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));

            return layer(finalX, finalY);
        };
    }
}
