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
#include "renderer/pipeline/ranges/CartesianRange.h"

namespace PolarShader {
    namespace {
        // Smoothing controls when zoom is at minimum (highest noise frequency).
        const int32_t TRANSLATION_SMOOTH_ALPHA_MIN = Q0_16_ONE / 16;
        const int32_t TRANSLATION_SMOOTH_ALPHA_MAX = Q0_16_ONE / 2;
    }

    struct TranslationTransform::State {
        CartesianRange velocityRange;
        CartesianMotionAccumulator motion;
        SPoint32 offset{0, 0};
        bool hasSmoothed{false};

        State(
            CartesianRange range,
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
        CartesianRange(),
        std::move(direction),
        std::move(velocity)
    )) {
    }

    void TranslationTransform::advanceFrame(TimeMillis timeInMillis) {
        SPoint32 target = state->motion.advance(timeInMillis);
        if (!state->hasSmoothed) {
            state->offset = target;
            state->hasSmoothed = true;
            return;
        }

        int32_t zoom_norm_raw = Q0_16_ONE;
        if (context) zoom_norm_raw = raw(context->zoomNormalized);

        int64_t alpha_span = static_cast<int64_t>(TRANSLATION_SMOOTH_ALPHA_MAX) -
                             static_cast<int64_t>(TRANSLATION_SMOOTH_ALPHA_MIN);
        int64_t alpha = static_cast<int64_t>(TRANSLATION_SMOOTH_ALPHA_MIN) +
                        ((alpha_span * static_cast<int64_t>(zoom_norm_raw)) >> 16);

        int64_t dx = static_cast<int64_t>(target.x) - static_cast<int64_t>(state->offset.x);
        int64_t dy = static_cast<int64_t>(target.y) - static_cast<int64_t>(state->offset.y);
        dx = (dx * alpha) >> 16;
        dy = (dy * alpha) >> 16;
        state->offset.x = static_cast<int32_t>(static_cast<int64_t>(state->offset.x) + dx);
        state->offset.y = static_cast<int32_t>(static_cast<int64_t>(state->offset.y) + dy);
    }

    CartesianLayer TranslationTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](CartQ24_8 x, CartQ24_8 y, uint32_t depth) {
            int32_t sx = static_cast<int32_t>(static_cast<int64_t>(raw(x)) + state->offset.x);
            int32_t sy = static_cast<int32_t>(static_cast<int64_t>(raw(y)) + state->offset.y);
            return layer(CartQ24_8(sx), CartQ24_8(sy), depth);
        };
    }
}
