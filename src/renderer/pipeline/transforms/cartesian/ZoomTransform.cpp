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

#include "ZoomTransform.h"
#include "renderer/pipeline/maths/Maths.h"
#include <Arduino.h>

namespace PolarShader {
    namespace {
        // Smaller scale => higher noise frequency (zooming out shows more detail).
        const int32_t MIN_SCALE = scale32(Q0_16_MAX, frac(160)); //0.00625x
        const int32_t MAX_SCALE = Q0_16_ONE * 4; //4x
        const int32_t ZOOM_SMOOTH_ALPHA_MIN = Q0_16_ONE / 32; // 0.03125 in Q0.16
        const int32_t ZOOM_SMOOTH_ALPHA_MAX = Q0_16_ONE;

        uint32_t clampFracRaw(int32_t raw_value) {
            if (raw_value <= 0) return 0u;
            if (raw_value >= static_cast<int32_t>(FRACT_Q0_16_MAX)) return FRACT_Q0_16_MAX;
            return static_cast<uint32_t>(raw_value);
        }
    }

    struct ZoomTransform::State {
        SFracQ0_16Signal scaleSignal;
        SFracQ0_16 scaleValue = SFracQ0_16(MIN_SCALE);

        explicit State(SFracQ0_16Signal s)
            : scaleSignal(std::move(s)) {
        }
    };

    ZoomTransform::ZoomTransform(SFracQ0_16Signal scale, ZoomAnchor anchor)
        : state(std::make_shared<State>(std::move(scale))),
          anchor(anchor) {
    }

    void ZoomTransform::advanceFrame(TimeMillis timeInMillis) {
        uint32_t t_raw = clampFracRaw(raw(state->scaleSignal(timeInMillis)));
        int64_t span = static_cast<int64_t>(MAX_SCALE) - static_cast<int64_t>(MIN_SCALE);
        int64_t target = static_cast<int64_t>(MIN_SCALE);
        if (anchor == ZoomAnchor::MidPoint) {
            int64_t half_span = span / 2;
            int64_t mid = static_cast<int64_t>(MIN_SCALE) + half_span;
            int32_t centered = static_cast<int32_t>(t_raw) - static_cast<int32_t>(U16_HALF);
            int64_t offset = (static_cast<int64_t>(centered) * half_span) / static_cast<int64_t>(U16_HALF);
            target = mid + offset;
        } else if (anchor == ZoomAnchor::Ceiling) {
            int64_t offset = (static_cast<int64_t>(t_raw) * span) >> 16;
            target = static_cast<int64_t>(MAX_SCALE) - offset;
        } else {
            int64_t offset = (static_cast<int64_t>(t_raw) * span) >> 16;
            target = static_cast<int64_t>(MIN_SCALE) + offset;
        }
        if (target < MIN_SCALE) target = MIN_SCALE;
        if (target > MAX_SCALE) target = MAX_SCALE;
        int32_t target_raw = static_cast<int32_t>(target);
        int32_t current_raw = raw(state->scaleValue);
        int32_t delta = target_raw - current_raw;
        int64_t alpha_span = static_cast<int64_t>(ZOOM_SMOOTH_ALPHA_MAX) - static_cast<int64_t>(ZOOM_SMOOTH_ALPHA_MIN);
        int64_t freq_bias = static_cast<int64_t>(MAX_SCALE) - static_cast<int64_t>(target_raw);

        if (freq_bias < 0) freq_bias = 0;
        if (span > 0 && freq_bias > span) freq_bias = span;

        // Increase smoothing as noise frequency rises (lower scale => lower alpha).
        int64_t alpha = ZOOM_SMOOTH_ALPHA_MAX;
        if (span > 0) alpha -= (alpha_span * freq_bias) / span;

        int64_t step = static_cast<int64_t>(delta) * alpha;
        step = (step >= 0) ? (step + U16_HALF) : (step - U16_HALF);
        step >>= 16;

        state->scaleValue = SFracQ0_16(static_cast<int32_t>(static_cast<int64_t>(current_raw) + step));
        if (context) {
            context->zoomScale = state->scaleValue;
            int64_t span_norm = static_cast<int64_t>(MAX_SCALE) - static_cast<int64_t>(MIN_SCALE);
            int64_t numer = static_cast<int64_t>(raw(state->scaleValue)) - static_cast<int64_t>(MIN_SCALE);
            if (span_norm <= 0) {
                context->zoomNormalized = SFracQ0_16(Q0_16_ONE);
            } else {
                if (numer < 0) numer = 0;
                if (numer > span_norm) numer = span_norm;
                int64_t t_raw = (numer << 16) / span_norm;
                if (t_raw < 0) t_raw = 0;
                if (t_raw > Q0_16_ONE) t_raw = Q0_16_ONE;
                context->zoomNormalized = SFracQ0_16(static_cast<int32_t>(t_raw));
            }
        } else {
            Serial.println("ZoomTransform::advanceFrame context is null.");
        }
    }

    CartesianLayer ZoomTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](CartQ24_8 x, CartQ24_8 y) {
            // Apply Q0.16 scale to Q24.8 coords, preserving fractional precision.
            int64_t sx = static_cast<int64_t>(raw(x)) * static_cast<int64_t>(raw(state->scaleValue));
            int64_t sy = static_cast<int64_t>(raw(y)) * static_cast<int64_t>(raw(state->scaleValue));

            int32_t finalX = static_cast<int32_t>(sx >> 16);
            int32_t finalY = static_cast<int32_t>(sy >> 16);
            return layer(CartQ24_8(finalX), CartQ24_8(finalY));
        };
    }
}
