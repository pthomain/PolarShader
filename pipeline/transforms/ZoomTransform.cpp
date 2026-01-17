//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2023 Pierre Thomain

/*
 * This file is part of LED Segments.
 *
 * LED Segments is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LED Segments is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LED Segments. If not, see <https://www.gnu.org/licenses/>.
 */

#include "ZoomTransform.h"
#include <cstring>
#include "polar/pipeline/signals/Fluctuation.h"
#include "polar/pipeline/utils/FixMathUtils.h"
#include "polar/pipeline/utils/MathUtils.h"

namespace LEDSegments {
    struct ZoomTransform::State {
        ScalarMotion scaleSignal;
        bool useOsc = false;
        Units::FracQ16_16 base;
        Units::FracQ16_16 amp;
        Units::FracQ16_16 phaseVel;
        Units::AngleTurnsUQ16_16 phase = 0;
        Units::TimeMillis lastTime = 0;

        explicit State(ScalarMotion s) : scaleSignal(std::move(s)) {}
        State(Units::FracQ16_16 b, Units::FracQ16_16 a, Units::FracQ16_16 pv, Units::AngleTurnsUQ16_16 p)
            : scaleSignal(ScalarMotion(Fluctuations::ConstantSignal(0))),
              useOsc(true),
              base(b),
              amp(a),
              phaseVel(pv),
              phase(p) {}
    };

    ZoomTransform::ZoomTransform(ScalarMotion scale)
        : state(std::make_shared<State>(std::move(scale))) {}

    ZoomTransform::ZoomTransform(Units::FracQ16_16 base,
                                 Units::FracQ16_16 amplitude,
                                 Units::FracQ16_16 phaseVelocity,
                                 Units::AngleTurnsUQ16_16 initialPhase)
        : state(std::make_shared<State>(base, amplitude, phaseVelocity, initialPhase)) {}

    void ZoomTransform::advanceFrame(Units::TimeMillis timeInMillis) {
        if (!state->useOsc) {
            state->scaleSignal.advanceFrame(timeInMillis);
            return;
        }
        if (state->lastTime == 0) {
            state->lastTime = timeInMillis;
            return;
        }
        Units::TimeMillis dt = timeInMillis - state->lastTime;
        state->lastTime = timeInMillis;
        Units::FracQ16_16 dt_q16 = millisToQ16_16(dt);
        Units::RawFracQ16_16 advance = mul_q16_16_wrap(state->phaseVel, dt_q16).asRaw();
        state->phase += static_cast<uint32_t>(advance);
    }

    CartesianLayer ZoomTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y) {
            Units::RawFracQ16_16 s_raw;
            if (state->useOsc) {
                Units::AngleUnitsQ0_16 phase_sample = static_cast<Units::AngleUnitsQ0_16>(state->phase >> 16);
                int32_t sin_val = sin16(phase_sample);
                // amp (Q16.16) * sin (Q1.15) -> Q17.31; shift 15 -> Q17.16
                int64_t delta = (static_cast<int64_t>(state->amp.asRaw()) * sin_val) >> 15;
                int64_t scaled = state->base.asRaw() + delta;
                // clamp to safe zoom bounds
                if (scaled < ZOOM_MIN.asRaw()) scaled = ZOOM_MIN.asRaw();
                if (scaled > ZOOM_MAX.asRaw()) scaled = ZOOM_MAX.asRaw();
                s_raw = static_cast<Units::RawFracQ16_16>(scaled);
            } else {
                s_raw = state->scaleSignal.getRawValue();
                if (s_raw < ZOOM_MIN.asRaw()) s_raw = ZOOM_MIN.asRaw();
                if (s_raw > ZOOM_MAX.asRaw()) s_raw = ZOOM_MAX.asRaw();
            }
            // Allow scale == 0 to collapse to origin; negative scales flip axes.
            int64_t sx = static_cast<int64_t>(x) * s_raw;
            int64_t sy = static_cast<int64_t>(y) * s_raw;

            uint32_t wrappedX = static_cast<uint32_t>(sx >> 16);
            uint32_t wrappedY = static_cast<uint32_t>(sy >> 16);

            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));
            return layer(finalX, finalY);
        };
    }
}
