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
#include "renderer/pipeline/maths/ZoomMaths.h"
#include "renderer/pipeline/ranges/ZoomRange.h"
#include <Arduino.h>

namespace PolarShader {
    namespace {
        const int32_t ZOOM_SMOOTH_ALPHA_MIN = Q0_16_ONE / 32; // 0.03125 in Q0.16
        const int32_t ZOOM_SMOOTH_ALPHA_MAX = Q0_16_ONE;
    }

    struct ZoomTransform::MappedInputs {
        MappedSignal<SFracQ0_16> scaleSignal;
        ZoomRange range;
    };

    struct ZoomTransform::State {
        MappedSignal<SFracQ0_16> scaleSignal;
        ZoomRange range;
        SFracQ0_16 scaleValue;
        int32_t minScaleRaw;
        int32_t maxScaleRaw;

        State(MappedSignal<SFracQ0_16> s, ZoomRange range)
            : scaleSignal(std::move(s)),
              range(std::move(range)),
              scaleValue(SFracQ0_16(0)),
              minScaleRaw(0),
              maxScaleRaw(0) {
            minScaleRaw = zoomMinScaleRaw(this->range);
            maxScaleRaw = zoomMaxScaleRaw(this->range);
            scaleValue = SFracQ0_16(minScaleRaw);
        }
    };

    ZoomTransform::ZoomTransform(
        MappedSignal<SFracQ0_16> scale,
        ZoomRange range
    ) : state(std::make_shared<State>(std::move(scale), std::move(range))) {
    }

    ZoomTransform::ZoomTransform(MappedInputs inputs)
        : ZoomTransform(std::move(inputs.scaleSignal), std::move(inputs.range)) {
    }

    ZoomTransform::MappedInputs ZoomTransform::makeInputs(
        SFracQ0_16Signal scale
    ) {
        ZoomRange range;
        return ZoomTransform::MappedInputs{
            range.mapSignal(std::move(scale)),
            std::move(range)
        };
    }

    ZoomTransform::ZoomTransform(SFracQ0_16Signal scale)
        : ZoomTransform(makeInputs(std::move(scale))) {
    }

    void ZoomTransform::advanceFrame(TimeMillis timeInMillis) {
        int32_t target_raw = raw(state->scaleSignal(timeInMillis).get());
        int32_t current_raw = raw(state->scaleValue);
        int32_t delta = target_raw - current_raw;
        int64_t alpha_span = static_cast<int64_t>(ZOOM_SMOOTH_ALPHA_MAX) - static_cast<int64_t>(ZOOM_SMOOTH_ALPHA_MIN);
        int32_t min_scale_raw = state->minScaleRaw;
        int32_t max_scale_raw = state->maxScaleRaw;
        int64_t span = static_cast<int64_t>(max_scale_raw) - static_cast<int64_t>(min_scale_raw);
        int64_t freq_bias = static_cast<int64_t>(max_scale_raw) - static_cast<int64_t>(target_raw);

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
            context->zoomNormalized = zoomNormalize(state->scaleValue, min_scale_raw, max_scale_raw);
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
