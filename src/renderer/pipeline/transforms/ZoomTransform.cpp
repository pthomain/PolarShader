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
#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif

namespace PolarShader {
    namespace {
        const int32_t ZOOM_SMOOTH_ALPHA_MIN = Q0_16_ONE / 32; // 0.03125 in Q0.16
        const int32_t ZOOM_SMOOTH_ALPHA_MAX = Q0_16_ONE;

        // Default Zoom scale boundaries
        const int32_t MIN_SCALE_RAW = Q0_16_ONE >> 4; // (1/16)x
        const int32_t MAX_SCALE_RAW = Q0_16_ONE << 4; // 16x (Zoomed Out)
    }

    struct ZoomTransform::MappedInputs {
        MappedSignal<SFracQ0_16> scaleSignal;
        LinearRange<SFracQ0_16> range;
    };

    ZoomTransform::MappedInputs ZoomTransform::makeInputs(SFracQ0_16Signal scale) {
        LinearRange range{SFracQ0_16(MIN_SCALE_RAW), SFracQ0_16(MAX_SCALE_RAW)};
        auto mapped = range.mapSignal(std::move(scale));
        return MappedInputs{
            mapped.withAbsolute(true),
            std::move(range)
        };
    }

    struct ZoomTransform::State {
        MappedSignal<SFracQ0_16> scaleSignal;
        LinearRange<SFracQ0_16> range;
        SFracQ0_16 scaleValue;
        int32_t minScaleRaw;
        int32_t maxScaleRaw;

        State(MappedSignal<SFracQ0_16> s, LinearRange<SFracQ0_16> range)
            : scaleSignal(std::move(s)),
              range(std::move(range)),
              scaleValue(SFracQ0_16(0)),
              minScaleRaw(0),
              maxScaleRaw(0) {
            minScaleRaw = this->range.minRaw();
            maxScaleRaw = this->range.maxRaw();
        }
    };

    ZoomTransform::ZoomTransform(SFracQ0_16Signal scale) {
        auto inputs = makeInputs(std::move(scale));
        state = std::make_shared<State>(std::move(inputs.scaleSignal), std::move(inputs.range));
    }

    void ZoomTransform::advanceFrame(FracQ0_16 progress, TimeMillis elapsedMs) {
        int32_t target_raw = raw(state->scaleSignal(progress, elapsedMs).get());
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
        } else {
            Serial.println("ZoomTransform::advanceFrame context is null.");
        }
    }

    UVMap ZoomTransform::operator()(const UVMap &layer) const {
        return [state = this->state, layer](UV uv) {
            // Map from [0, 1] to [-1, 1] (relative to center)
            int64_t x = (static_cast<int64_t>(raw(uv.u)) << 1) - 0x00010000;
            int64_t y = (static_cast<int64_t>(raw(uv.v)) << 1) - 0x00010000;

            // Apply Q0.16 scale
            int64_t sx = x * static_cast<int64_t>(raw(state->scaleValue));
            int64_t sy = y * static_cast<int64_t>(raw(state->scaleValue));

            // Shift back down
            int32_t fx = static_cast<int32_t>(sx >> 16);
            int32_t fy = static_cast<int32_t>(sy >> 16);

            // Map from [-1, 1] to [0, 1]
            UV scaled_uv(
                FracQ16_16((fx + 0x00010000) >> 1),
                FracQ16_16((fy + 0x00010000) >> 1)
            );

            return layer(scaled_uv);
        };
    }
}
