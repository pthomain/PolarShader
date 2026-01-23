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

#include "BendTransform.h"
#include <cstring>
#include "../modulators/BoundUtils.h"
#include "renderer/pipeline/utils/MathUtils.h"

namespace PolarShader {
    namespace {
        constexpr UnboundedScalar kBendMin = UnboundedScalar::fromRaw(-static_cast<int32_t>(Q16_16_ONE / 32));
        constexpr UnboundedScalar kBendMax = UnboundedScalar::fromRaw(Q16_16_ONE / 32);
    }

    struct BendTransform::State {
        BoundedScalarSignal kxSignal;
        BoundedScalarSignal kySignal;
        UnboundedScalar kxValue = kBendMin;
        UnboundedScalar kyValue = kBendMin;

        State(BoundedScalarSignal kx, BoundedScalarSignal ky)
            : kxSignal(std::move(kx)), kySignal(std::move(ky)) {
        }
    };

    BendTransform::BendTransform(BoundedScalarSignal kx, BoundedScalarSignal ky)
        : state(std::make_shared<State>(std::move(kx), std::move(ky))) {
    }

    void BendTransform::advanceFrame(TimeMillis timeInMillis) {
        state->kxValue = unbound(state->kxSignal(timeInMillis), kBendMin, kBendMax);
        state->kyValue = unbound(state->kySignal(timeInMillis), kBendMin, kBendMax);
    }

    CartesianLayer BendTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y) {
            RawQ16_16 kx_raw = RawQ16_16(state->kxValue.asRaw());
            RawQ16_16 ky_raw = RawQ16_16(state->kyValue.asRaw());

            auto safe_bend = [](RawQ16_16 k_raw, int32_t coord) -> int64_t {
                int32_t k_val = raw(k_raw);
                if (k_val == 0) return 0;
                int64_t squared = static_cast<int64_t>(coord) * static_cast<int64_t>(coord);
                int64_t abs_k = std::llabs(k_val);
                int64_t limit = (abs_k == 0) ? 0 : (INT64_MAX / abs_k);
                if (squared > limit) squared = limit;
                return (squared * k_val) >> 16;
            };

            int64_t bendX = safe_bend(kx_raw, y);
            int64_t bendY = safe_bend(ky_raw, x);

            uint32_t wrappedX = static_cast<uint32_t>(static_cast<int64_t>(x) + bendX);
            uint32_t wrappedY = static_cast<uint32_t>(static_cast<int64_t>(y) + bendY);

            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));

            return layer(finalX, finalY);
        };
    }
}
