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

#include "LensDistortionTransform.h"
#include "../modulators/BoundUtils.h"
#include "renderer/pipeline/utils/MathUtils.h"

namespace PolarShader {
    namespace {
        constexpr UnboundedScalar kDistortMin = UnboundedScalar::fromRaw(-static_cast<int32_t>(Q16_16_ONE / 8));
        constexpr UnboundedScalar kDistortMax = UnboundedScalar::fromRaw(Q16_16_ONE / 8);
    }

    struct LensDistortionTransform::State {
        BoundedScalarSignal kSignal;
        UnboundedScalar kValue = kDistortMin;

        explicit State(BoundedScalarSignal k) : kSignal(std::move(k)) {
        }
    };

    LensDistortionTransform::LensDistortionTransform(BoundedScalarSignal k)
        : state(std::make_shared<State>(std::move(k))) {
    }

    void LensDistortionTransform::advanceFrame(TimeMillis timeInMillis) {
        state->kValue = unbound(state->kSignal(timeInMillis), kDistortMin, kDistortMax);
    }

    PolarLayer LensDistortionTransform::operator()(const PolarLayer &layer) const {
        return [state = this->state, layer](UnboundedAngle angle_q16, BoundedScalar radius) {
            RawQ16_16 k_raw = RawQ16_16(state->kValue.asRaw()); // Q16.16
            // factor = 1 + k * r
            int64_t factor_q16 = static_cast<int64_t>(Q16_16_ONE)
                                 + ((static_cast<int64_t>(raw(k_raw)) * raw(radius)) >> 16);
            int64_t scaled = factor_q16 * raw(radius); // Q16.16 * Q0.16 = Q16.32
            scaled = (scaled + (1LL << 15)) >> 16; // round to Q16.16
            if (scaled < 0) scaled = 0;
            if (scaled > FRACT_Q0_16_MAX) scaled = FRACT_Q0_16_MAX;
            BoundedScalar r_out = BoundedScalar(static_cast<uint16_t>(scaled));
            return layer(angle_q16, r_out);
        };
    }
}
