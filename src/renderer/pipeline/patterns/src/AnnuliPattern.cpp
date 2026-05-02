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

#include "renderer/pipeline/patterns/AnnuliPattern.h"
#include "renderer/pipeline/maths/PolarMaths.h"
#include <algorithm>

namespace PolarShader {
    namespace {
        constexpr uint8_t kMinRings = 1;
        constexpr uint8_t kMinSlices = 1;
        constexpr uint16_t kMinStepIntervalMs = 1;
    }

    struct AnnuliPattern::State {
        uint8_t ringCount;
        uint8_t slicesPerRing;
        bool reverse;
        uint16_t stepIntervalMs;
        uint16_t holdMs;

        // Total cells in one full sweep.
        uint16_t totalCells;

        // State machine.
        // counter < 0   : not yet started (first frame).
        // 0..total-1    : fill index of the most-recently lit cell.
        // == total      : sweep complete, holding fully-lit.
        int32_t counter;
        uint32_t tickAccumMs;
        uint32_t holdAccumMs;
        TimeMillis lastElapsedMs;
        bool hasLastElapsed;

        State(
            uint8_t rings,
            uint8_t slices,
            bool reverse,
            uint16_t stepMs,
            uint16_t holdMs
        ) : ringCount(std::max(kMinRings, rings)),
            slicesPerRing(std::max(kMinSlices, slices)),
            reverse(reverse),
            stepIntervalMs(std::max(kMinStepIntervalMs, stepMs)),
            holdMs(holdMs),
            totalCells(static_cast<uint16_t>(static_cast<uint32_t>(ringCount) * slicesPerRing)),
            counter(-1),
            tickAccumMs(0u),
            holdAccumMs(0u),
            lastElapsedMs(0u),
            hasLastElapsed(false) {}
    };

    struct AnnuliPattern::UVAnnuliFunctor {
        const State *state;

        PatternNormU16 operator()(UV uv) const {
            const State &s = *state;
            if (s.counter < 0) return PatternNormU16(0u);

            // Convert UV to polar: out.u = angle (f16 turns), out.v = radius (q16 [0, 1]).
            UV polar = cartesianToPolarUV(uv);
            uint16_t angleTurns = static_cast<uint16_t>(polar.u.raw());
            uint32_t radiusRaw = static_cast<uint32_t>(polar.v.raw());

            // ringIndex: 0 = innermost. sliceIndex: angular bin.
            uint32_t ringIndex = (radiusRaw * s.ringCount) >> 16;
            uint32_t sliceIndex = (static_cast<uint32_t>(angleTurns) * s.slicesPerRing) >> 16;

            // Forward fill: outermost ring (highest ringIndex) gets fillRing 0 (drawn first).
            // Reverse fill: innermost ring (ringIndex 0) gets fillRing 0.
            uint32_t fillRing = s.reverse
                ? ringIndex
                : static_cast<uint32_t>(s.ringCount - 1u - ringIndex);
            uint32_t cellIndex = fillRing * s.slicesPerRing + sliceIndex;

            if (s.counter < static_cast<int32_t>(cellIndex)) return PatternNormU16(0u);

            // Filled cell: emit angle as intensity so the palette wraps as a colour wheel.
            return PatternNormU16(angleTurns);
        }
    };

    AnnuliPattern::AnnuliPattern(
        uint8_t ringCount,
        uint8_t slicesPerRing,
        bool reverse,
        uint16_t stepIntervalMs,
        uint16_t holdMs
    ) : state(std::make_shared<State>(
        ringCount,
        slicesPerRing,
        reverse,
        stepIntervalMs,
        holdMs
    )) {}

    void AnnuliPattern::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void)progress;
        State &s = *state;

        // Delta-time in ms (clamped).
        TimeMillis dtMs = 0u;
        if (!s.hasLastElapsed) {
            s.lastElapsedMs = elapsedMs;
            s.hasLastElapsed = true;
            // Initialise counter on first frame.
            s.counter = 0;
            s.tickAccumMs = 0u;
            s.holdAccumMs = 0u;
            return;
        }
        dtMs = elapsedMs >= s.lastElapsedMs ? (elapsedMs - s.lastElapsedMs) : 0u;
        if (dtMs > MAX_DELTA_TIME_MS) dtMs = MAX_DELTA_TIME_MS;
        s.lastElapsedMs = elapsedMs;

        if (s.counter >= static_cast<int32_t>(s.totalCells)) {
            // Holding phase.
            s.holdAccumMs += static_cast<uint32_t>(dtMs);
            if (s.holdAccumMs >= s.holdMs) {
                s.counter = 0;
                s.tickAccumMs = 0u;
                s.holdAccumMs = 0u;
            }
            return;
        }

        // Running phase.
        s.tickAccumMs += static_cast<uint32_t>(dtMs);
        while (s.tickAccumMs >= s.stepIntervalMs && s.counter < static_cast<int32_t>(s.totalCells)) {
            s.tickAccumMs -= s.stepIntervalMs;
            s.counter += 1;
        }
        if (s.counter >= static_cast<int32_t>(s.totalCells)) {
            // Just transitioned to holding.
            s.tickAccumMs = 0u;
            s.holdAccumMs = 0u;
        }
    }

    UVMap AnnuliPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        (void)context;
        return UVAnnuliFunctor{state.get()};
    }
}
