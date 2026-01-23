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

#include "RadialScaleTransform.h"
#include "../modulators/BoundUtils.h"
#include "renderer/pipeline/utils/MathUtils.h"

namespace PolarShader {
    namespace {
        constexpr UnboundedScalar kScaleMin = UnboundedScalar::fromRaw(-static_cast<int32_t>(Q16_16_ONE / 2));
        constexpr UnboundedScalar kScaleMax = UnboundedScalar::fromRaw(Q16_16_ONE / 2);
    }

    struct RadialScaleTransform::State {
        BoundedAngleSignal kSignal;
        UnboundedScalar kValue = kScaleMin;

        explicit State(BoundedAngleSignal k) : kSignal(std::move(k)) {
        }
    };

    RadialScaleTransform::RadialScaleTransform(BoundedAngleSignal k)
        : state(std::make_shared<State>(std::move(k))) {
    }

    void RadialScaleTransform::advanceFrame(TimeMillis timeInMillis) {
        BoundedScalar k_bounded = BoundedScalar(raw(state->kSignal(timeInMillis)));
        state->kValue = unbound(k_bounded, kScaleMin, kScaleMax);
    }

    PolarLayer RadialScaleTransform::operator()(const PolarLayer &layer) const {
        return [state = this->state, layer](UnboundedAngle angle_q16, BoundedScalar radius) {
            RawQ16_16 k_raw = RawQ16_16(state->kValue.asRaw());

            int64_t delta = (static_cast<int64_t>(raw(k_raw)) * static_cast<int64_t>(raw(radius))) >> 16;
            int64_t new_radius = static_cast<int64_t>(raw(radius)) + delta;

            if (new_radius < 0) new_radius = 0;
            if (new_radius > FRACT_Q0_16_MAX) new_radius = FRACT_Q0_16_MAX;

            BoundedScalar r_out = BoundedScalar(static_cast<uint16_t>(new_radius));
            return layer(angle_q16, r_out);
        };
    }
}
