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
#include <cstdio>
#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif

namespace PolarShader {
    namespace {
        // Default Zoom scale boundaries
        const int32_t MIN_SCALE_RAW = Q0_16_ONE >> 4; // (1/16)x
        const int32_t MAX_SCALE_RAW = Q0_16_ONE << 4; // 16x (Zoomed Out)
    }

    struct ZoomTransform::MappedInputs {
        SFracQ0_16Signal scaleSignal;
        LinearRange<SFracQ0_16> range;
    };

    ZoomTransform::MappedInputs ZoomTransform::makeInputs(SFracQ0_16Signal scale) {
        LinearRange range{SFracQ0_16(MIN_SCALE_RAW), SFracQ0_16(MAX_SCALE_RAW)};
        return MappedInputs{
            std::move(scale),
            std::move(range)
        };
    }

    struct ZoomTransform::State {
        SFracQ0_16Signal scaleSignal;
        LinearRange<SFracQ0_16> range;
        SFracQ0_16 scaleValue;
        int32_t minScaleRaw;
        int32_t maxScaleRaw;
        TimeMillis lastLogMs;

        explicit State(MappedInputs inputs)
            : scaleSignal(std::move(inputs.scaleSignal)),
              range(std::move(inputs.range)),
              scaleValue(SFracQ0_16(0)),
              minScaleRaw(0),
              maxScaleRaw(0),
              lastLogMs(0) {
            minScaleRaw = this->range.minRaw();
            maxScaleRaw = this->range.maxRaw();
        }
    };

    ZoomTransform::ZoomTransform(SFracQ0_16Signal scale) {
        auto inputs = makeInputs(std::move(scale));
        state = std::make_shared<State>(std::move(inputs));
    }

    void ZoomTransform::advanceFrame(FracQ0_16 progress, TimeMillis elapsedMs) {
        state->scaleValue = state->scaleSignal.sample(state->range, elapsedMs);
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
            int32_t scale = raw(state->scaleValue);

            // Apply Q0.16 scale
            int64_t sx = x * static_cast<int64_t>(scale);
            int64_t sy = y * static_cast<int64_t>(scale);

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
