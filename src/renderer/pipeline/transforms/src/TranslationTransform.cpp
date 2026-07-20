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

#include "renderer/pipeline/transforms/TranslationTransform.h"
#include "renderer/pipeline/maths/Maths.h"
#if defined(ARDUINO) || defined(__EMSCRIPTEN__)
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif
#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/transforms/base/Transforms.h"
#include "renderer/pipeline/signals/ranges/AngleRange.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"

namespace PolarShader {
    namespace {
        // Smoothing controls when zoom is at minimum (highest noise frequency).
        const int32_t TRANSLATION_SMOOTH_ALPHA_MIN = S0X16_ONE / 16;
        const int32_t TRANSLATION_SMOOTH_ALPHA_MAX = S0X16_ONE / 2;
        const int32_t TRANSLATION_SMOOTH_SCALE_MIN = S0X16_ONE >> 4; // 1/16x
        const int32_t TRANSLATION_SMOOTH_SCALE_MAX = S0X16_ONE << 4; // 16x
        const int32_t TRANSLATION_MAX_SPEED = S0X16_ONE >> 2; // 1/4 of UV units in fl::s16x16 (Q16.16) raw domain

        UVSignal accumulateUVSignal(UVSignal signal) {
            return UVSignal(
                [signal = std::move(signal), accumulated = UV(fl::s16x16::from_raw(0), fl::s16x16::from_raw(0))](
            u0x16 progress,
            TimeMillis elapsedMs
        ) mutable {
                    UV value = signal(progress, elapsedMs);
                    accumulated.u = accumulated.u + value.u;
                    accumulated.v = accumulated.v + value.v;
                    return accumulated;
                }
            );
        }
    }

    struct TranslationTransform::State {
        UVSignal offsetSignal;
        UV offset{fl::s16x16::from_raw(0), fl::s16x16::from_raw(0)};
        bool hasSmoothed{false};

        State(UVSignal signal) : offsetSignal(std::move(signal)) {
        }
    };

    TranslationTransform::TranslationTransform(UVSignal signal)
        : state(std::make_shared<State>(std::move(signal))) {
    }

    TranslationTransform::TranslationTransform(
        S0x16Signal direction,
        S0x16Signal speed
    ) : TranslationTransform(accumulateUVSignal(UVSignal([direction, speed]() {
        // Map speed signal to fl::s16x16 velocity magnitude [0 .. 0.25 UV units].
        MagnitudeRange speedRange(fl::s16x16::from_raw(0), fl::s16x16::from_raw(TRANSLATION_MAX_SPEED));

        // Use a lambda to combine direction and speed into a velocity UV vector
        // This is a per-frame delta signal that is accumulated into absolute offset.
        AngleRange directionRange;
        return UVSignal([direction, speed, speedRange, directionRange](u0x16 progress, TimeMillis elapsedMs) mutable {
            u0x16 dir = direction.sample(directionRange, elapsedMs);
            int32_t s = speed.sample(speedRange, elapsedMs).raw();

            s0x16 cos_val = angleCosU0x16(dir);
            s0x16 sin_val = angleSinU0x16(dir);

            // Velocity vector in UV units (fl::u16x16/fl::s16x16 (Q16.16)) per scene
            // Trig is u0x16/s0x16, s is fl::u16x16/fl::s16x16 (Q16.16) units/scene.
            // Result is fl::u16x16/fl::s16x16 (Q16.16).
            int32_t vx = static_cast<int32_t>((static_cast<int64_t>(s) * raw(cos_val)) >> 16);
            int32_t vy = static_cast<int32_t>((static_cast<int64_t>(s) * raw(sin_val)) >> 16);

            return UV(fl::s16x16::from_raw(vx), fl::s16x16::from_raw(vy));
        });
    }()))) {
    }

    void TranslationTransform::advanceFrame(u0x16 progress, TimeMillis elapsedMs) {
        UV target = state->offsetSignal(progress, elapsedMs);
        if (!state->hasSmoothed) {
            state->offset = target;
            state->hasSmoothed = true;
            return;
        }

        int32_t zoom_raw = context ? raw(context->zoomScale) : S0X16_ONE;
        if (zoom_raw < TRANSLATION_SMOOTH_SCALE_MIN) zoom_raw = TRANSLATION_SMOOTH_SCALE_MIN;
        if (zoom_raw > TRANSLATION_SMOOTH_SCALE_MAX) zoom_raw = TRANSLATION_SMOOTH_SCALE_MAX;
        int32_t zoom_span = TRANSLATION_SMOOTH_SCALE_MAX - TRANSLATION_SMOOTH_SCALE_MIN;
        int32_t zoom_norm = zoom_span > 0 ? ((zoom_raw - TRANSLATION_SMOOTH_SCALE_MIN) << 16) / zoom_span : S0X16_ONE;

        int64_t alpha_span = static_cast<int64_t>(TRANSLATION_SMOOTH_ALPHA_MAX) -
                             static_cast<int64_t>(TRANSLATION_SMOOTH_ALPHA_MIN);
        int64_t alpha = static_cast<int64_t>(TRANSLATION_SMOOTH_ALPHA_MIN) +
                        ((alpha_span * zoom_norm) >> 16);

        int64_t du = static_cast<int64_t>(target.u.raw()) - static_cast<int64_t>(state->offset.u.raw());
        int64_t dv = static_cast<int64_t>(target.v.raw()) - static_cast<int64_t>(state->offset.v.raw());
        du = (du * alpha) >> 16;
        dv = (dv * alpha) >> 16;

        state->offset.u = fl::s16x16::from_raw(static_cast<int32_t>(static_cast<int64_t>(state->offset.u.raw()) + du));
        state->offset.v = fl::s16x16::from_raw(static_cast<int32_t>(static_cast<int64_t>(state->offset.v.raw()) + dv));
    }

    UV TranslationTransform::warp(const State &state, UV uv) {
        return UV(
            uv.u + state.offset.u,
            uv.v + state.offset.v
        );
    }

    UVLayer TranslationTransform::apply(const UVLayer &layer) const {
        return composeUvLayer(layer, state, [](const State &state, UV uv) {
            return warp(state, uv);
        });
    }
}
