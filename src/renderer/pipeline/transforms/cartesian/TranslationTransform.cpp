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
#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif
#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/transforms/base/Transforms.h"
#include "renderer/pipeline/ranges/PolarRange.h"
#include "renderer/pipeline/ranges/ScalarRange.h"

namespace PolarShader {
    struct TranslationTransform::MappedInputs {
        MappedSignal<FracQ0_16> directionSignal;
        MappedSignal<int32_t> speedSignal;
    };

    namespace {
        // Smoothing controls when zoom is at minimum (highest noise frequency).
        const int32_t TRANSLATION_SMOOTH_ALPHA_MIN = Q0_16_ONE / 16;
        const int32_t TRANSLATION_SMOOTH_ALPHA_MAX = Q0_16_ONE / 2;
        const int32_t TRANSLATION_MAX_SPEED = static_cast<int32_t>(UINT32_MAX >> 5);
    }

    struct TranslationTransform::State {
        CartesianMotionAccumulator motion;
        SPoint32 offset{0, 0};
        bool hasSmoothed{false};

        State(
            MappedSignal<FracQ0_16> direction,
            MappedSignal<int32_t> speed
        ) : motion(
            SPoint32{0, 0},
            std::move(direction),
            std::move(speed)
        ) {
        }
    };

    TranslationTransform::TranslationTransform(
        MappedSignal<FracQ0_16> direction,
        MappedSignal<int32_t> speed
    ) : state(std::make_shared<State>(std::move(direction), std::move(speed))) {
    }

    TranslationTransform::TranslationTransform(MappedInputs inputs)
        : TranslationTransform(std::move(inputs.directionSignal), std::move(inputs.speedSignal)) {
    }

    TranslationTransform::MappedInputs TranslationTransform::makeInputs(
        SFracQ0_16Signal direction,
        SFracQ0_16Signal speed
    ) {
        return MappedInputs{
            resolveMappedSignal(PolarRange().mapSignal(std::move(direction))),
            resolveMappedSignal(ScalarRange(0, TRANSLATION_MAX_SPEED).mapSignal(std::move(speed)))
        };
    }

    TranslationTransform::TranslationTransform(
        SFracQ0_16Signal direction,
        SFracQ0_16Signal speed
    ) : TranslationTransform(makeInputs(std::move(direction), std::move(speed))) {
    }

    void TranslationTransform::advanceFrame(TimeMillis timeInMillis) {
        SPoint32 target = state->motion.advance(timeInMillis);
        if (!state->hasSmoothed) {
            state->offset = target;
            state->hasSmoothed = true;
            return;
        }

        int32_t zoom_norm_raw = Q0_16_ONE;
        if (context) {
            zoom_norm_raw = raw(context->zoomNormalized);
        } else {
            Serial.println("TranslationTransform::advanceFrame context is null.");
        }

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
        return [state = this->state, layer](CartQ24_8 x, CartQ24_8 y) {
            int32_t sx = static_cast<int32_t>(static_cast<int64_t>(raw(x)) + state->offset.x);
            int32_t sy = static_cast<int32_t>(static_cast<int64_t>(raw(y)) + state->offset.y);
            return layer(CartQ24_8(sx), CartQ24_8(sy));
        };
    }

    UVLayer TranslationTransform::operator()(const UVLayer &layer) const {
        return [state = this->state, layer](UV uv) {
            // Apply translation directly to UV coordinates (Q16.16)
            // state->offset is typically in CartQ24.8, so we convert it to UV (Q16.16)
            // by shifting left by 8.
            int32_t uv_ox = state->offset.x << 8;
            int32_t uv_oy = state->offset.y << 8;
            
            UV translated_uv(
                FracQ16_16(raw(uv.u) + uv_ox),
                FracQ16_16(raw(uv.v) + uv_oy)
            );
            return layer(translated_uv);
        };
    }
}
