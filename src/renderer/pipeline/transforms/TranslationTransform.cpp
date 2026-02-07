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
#include "renderer/pipeline/ranges/LinearRange.h"

namespace PolarShader {
    namespace {
        // Smoothing controls when zoom is at minimum (highest noise frequency).
        const int32_t TRANSLATION_SMOOTH_ALPHA_MIN = Q0_16_ONE / 16;
        const int32_t TRANSLATION_SMOOTH_ALPHA_MAX = Q0_16_ONE / 2;
        const int32_t TRANSLATION_MAX_SPEED = 1000; // units per second in Q16.16 domain

        UVSignal resolveTranslationSignal(UVSignal signal) {
            return resolveMappedSignal(std::move(signal));
        }
    }

    struct TranslationTransform::State {
        UVSignal offsetSignal;
        UV offset{FracQ16_16(0), FracQ16_16(0)};
        bool hasSmoothed{false};

        State(UVSignal signal) : offsetSignal(std::move(signal)) {}
    };

    TranslationTransform::TranslationTransform(UVSignal signal)
        : state(std::make_shared<State>(resolveTranslationSignal(std::move(signal)))) {
    }

    TranslationTransform::TranslationTransform(
        SFracQ0_16Signal direction,
        SFracQ0_16Signal speed
    ) : TranslationTransform(UVSignal([direction, speed]() {
        // Map speed 0..1 to 0..TRANSLATION_MAX_SPEED
        auto mappedSpeed = LinearRange<int32_t>(0, TRANSLATION_MAX_SPEED).mapSignal(speed);
        
        // Use a lambda to combine direction and speed into a velocity UV vector
        // This is a relative signal (absolute=false)
        return UVSignal([direction, mappedSpeed](FracQ0_16 progress, TimeMillis elapsedMs) mutable {
            FracQ0_16 dir = PolarRange().map(direction(progress, elapsedMs)).get();
            int32_t s = mappedSpeed(progress, elapsedMs).get();
            
            SFracQ0_16 cos_val = angleCosQ0_16(dir);
            SFracQ0_16 sin_val = angleSinQ0_16(dir);
            
            // Velocity vector in UV units (Q16.16) per scene
            // Trig is Q0.16, s is Q16.16 units/scene. 
            // Result is Q16.16.
            int32_t vx = static_cast<int32_t>((static_cast<int64_t>(s) * raw(cos_val)) >> 16);
            int32_t vy = static_cast<int32_t>((static_cast<int64_t>(s) * raw(sin_val)) >> 16);
            
            return UV(FracQ16_16(vx), FracQ16_16(vy));
        }, false);
    }())) {
    }

    void TranslationTransform::advanceFrame(FracQ0_16 progress, TimeMillis elapsedMs) {
        UV target = state->offsetSignal(progress, elapsedMs);
        if (!state->hasSmoothed) {
            state->offset = target;
            state->hasSmoothed = true;
            return;
        }

        int64_t alpha_span = static_cast<int64_t>(TRANSLATION_SMOOTH_ALPHA_MAX) -
                             static_cast<int64_t>(TRANSLATION_SMOOTH_ALPHA_MIN);
        int64_t alpha = static_cast<int64_t>(TRANSLATION_SMOOTH_ALPHA_MIN) +
                        ((alpha_span * raw(context->zoomScale)) >> 16);

        int64_t du = static_cast<int64_t>(raw(target.u)) - static_cast<int64_t>(raw(state->offset.u));
        int64_t dv = static_cast<int64_t>(raw(target.v)) - static_cast<int64_t>(raw(state->offset.v));
        du = (du * alpha) >> 16;
        dv = (dv * alpha) >> 16;

        state->offset.u = FracQ16_16(static_cast<int32_t>(static_cast<int64_t>(raw(state->offset.u)) + du));
        state->offset.v = FracQ16_16(static_cast<int32_t>(static_cast<int64_t>(raw(state->offset.v)) + dv));
    }

    UVMap TranslationTransform::operator()(const UVMap &layer) const {
        return [state = this->state, layer](UV uv) {
            UV translated_uv(
                FracQ16_16(raw(uv.u) + raw(state->offset.u)),
                FracQ16_16(raw(uv.v) + raw(state->offset.v))
            );
            return layer(translated_uv);
        };
    }
}
