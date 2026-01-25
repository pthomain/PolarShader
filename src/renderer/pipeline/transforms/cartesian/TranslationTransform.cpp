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

#include "TranslationTransform.h"
#include "renderer/pipeline/maths/Maths.h"
#include "renderer/pipeline/transforms/base/Transforms.h"

namespace PolarShader {
    struct TranslationTransform::State {
        Range velocityRange;
        CartesianMotionAccumulator motion;
        SPoint32 offset{0, 0};

        State(
            Range range,
            SFracQ0_16Signal direction,
            SFracQ0_16Signal velocity
        ) : velocityRange(std::move(range)),
            motion(
                SPoint32{0, 0},
                velocityRange,
                std::move(direction),
                std::move(velocity)
            ) {
        }
    };

    TranslationTransform::TranslationTransform(
        SFracQ0_16Signal direction,
        SFracQ0_16Signal velocity
    ) : state(std::make_shared<State>(
        Range::cartesianRange(),
        std::move(direction),
        std::move(velocity)
    )) {
    }

    void TranslationTransform::advanceFrame(TimeMillis timeInMillis) {
        state->offset = state->motion.advance(timeInMillis);
    }

    CartesianLayer TranslationTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](CartQ24_8 x, CartQ24_8 y) {
            int32_t sx = static_cast<int32_t>(static_cast<int64_t>(raw(x)) + state->offset.x);
            int32_t sy = static_cast<int32_t>(static_cast<int64_t>(raw(y)) + state->offset.y);
            return layer(CartQ24_8(sx), CartQ24_8(sy));
        };
    }
}
