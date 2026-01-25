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

namespace PolarShader {
    namespace {
        // Smaller scale => higher noise frequency (zooming out shows more detail).
        const int32_t MIN_SCALE = scale32(Q0_16_MAX, frac(160)); //0.00625x
        const int32_t MAX_SCALE = Q0_16_ONE * 4; //4x
        const int32_t ZOOM_SMOOTH_ALPHA_MIN = Q0_16_ONE / 32; // 0.03125 in Q0.16
        const int32_t ZOOM_SMOOTH_ALPHA_MAX = Q0_16_ONE;
    }

    struct ZoomTransform::State {
        SFracQ0_16Signal scaleSignal;
        SFracQ0_16 scaleValue = SFracQ0_16(MIN_SCALE);

        explicit State(SFracQ0_16Signal s)
            : scaleSignal(std::move(s)) {
        }
    };

    ZoomTransform::ZoomTransform(SFracQ0_16Signal scale)
        : CartesianTransform(Range::scalarRange(MIN_SCALE, MAX_SCALE)),
          state(std::make_shared<State>(std::move(scale))) {
    }

    void ZoomTransform::advanceFrame(TimeMillis timeInMillis) {
        int32_t target_raw = mapScalar(state->scaleSignal(timeInMillis));
        int32_t current_raw = raw(state->scaleValue);
        int32_t delta = target_raw - current_raw;
        int64_t span = static_cast<int64_t>(MAX_SCALE) - static_cast<int64_t>(MIN_SCALE);
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
