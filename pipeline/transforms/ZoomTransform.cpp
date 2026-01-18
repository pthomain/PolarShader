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
#include "polar/pipeline/signals/modulators/ScalarModulators.h"
#include "polar/pipeline/utils/FixMathUtils.h"
#include "polar/pipeline/utils/MathUtils.h"

namespace LEDSegments {
    struct ZoomTransform::State {
        ScalarMotion scaleSignal;
        bool useOsc = false;
        FracQ16_16 base;
        FracQ16_16 amp;
        FracQ16_16 phaseVel;
        AngleTurnsUQ16_16 phase = AngleTurnsUQ16_16(0);
        TimeMillis lastTime = 0;
        bool hasLastTime = false;

        explicit State(ScalarMotion s) : scaleSignal(std::move(s)) {
        }

        State(FracQ16_16 b, FracQ16_16 a, FracQ16_16 pv, AngleTurnsUQ16_16 p)
            : scaleSignal(ScalarMotion(Constant(FracQ16_16(0)))),
              useOsc(true),
              base(b),
              amp(a),
              phaseVel(pv),
              phase(p) {
        }
    };

    ZoomTransform::ZoomTransform(ScalarMotion scale)
        : state(std::make_shared<State>(std::move(scale))) {
    }

    ZoomTransform::ZoomTransform(FracQ16_16 base,
                                 FracQ16_16 amplitude,
                                 FracQ16_16 phaseVelocity,
                                 AngleTurnsUQ16_16 initialPhase)
        : state(std::make_shared<State>(base, amplitude, phaseVelocity, initialPhase)) {
    }

    void ZoomTransform::advanceFrame(TimeMillis timeInMillis) {
        if (!state->useOsc) {
            state->scaleSignal.advanceFrame(timeInMillis);
            return;
        }
        if (!state->hasLastTime) {
            state->lastTime = timeInMillis;
            state->hasLastTime = true;
            return;
        }
        TimeMillis dt = timeInMillis - state->lastTime;
        state->lastTime = timeInMillis;
        FracQ16_16 dt_q16 = millisToQ16_16(dt);
        RawQ16_16 advance = RawQ16_16(mul_q16_16_wrap(state->phaseVel, dt_q16).asRaw());
        state->phase = wrapAddSigned(state->phase, raw(advance));
    }

    CartesianLayer ZoomTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y) {
            RawQ16_16 s_raw = RawQ16_16(0);
            if (state->useOsc) {
                AngleUnitsQ0_16 phase_sample = angleTurnsToAngleUnits(state->phase);
                TrigQ1_15 sin_val = sinQ1_15(phase_sample);
                FracQ16_16 delta = mulTrigQ1_15_SignalQ16_16(sin_val, state->amp);
                int64_t scaled = state->base.asRaw() + delta.asRaw();
                // clamp to safe zoom bounds
                if (scaled < ZOOM_MIN.asRaw()) scaled = ZOOM_MIN.asRaw();
                if (scaled > ZOOM_MAX.asRaw()) scaled = ZOOM_MAX.asRaw();
                s_raw = RawQ16_16(static_cast<int32_t>(scaled));
            } else {
                s_raw = state->scaleSignal.getRawValue();
                if (raw(s_raw) < ZOOM_MIN.asRaw()) s_raw = RawQ16_16(ZOOM_MIN.asRaw());
                if (raw(s_raw) > ZOOM_MAX.asRaw()) s_raw = RawQ16_16(ZOOM_MAX.asRaw());
            }
            // Allow scale == 0 to collapse to origin; negative scales flip axes.
            int64_t sx = static_cast<int64_t>(x) * raw(s_raw);
            int64_t sy = static_cast<int64_t>(y) * raw(s_raw);

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
