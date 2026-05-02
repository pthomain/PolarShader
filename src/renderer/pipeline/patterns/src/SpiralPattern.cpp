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

#include "renderer/pipeline/patterns/SpiralPattern.h"
#include "renderer/pipeline/maths/AngleMaths.h"
#include "renderer/pipeline/maths/PolarMaths.h"
#include <algorithm>

namespace PolarShader {
    namespace {
        // Tightness signal [0, 1] is doubled to [0, 2] full turns of winding at radius=1.
        // Signal raw is sf16 (q16, 65536 = 1.0); raw<<1 yields q16 turn-units in [0, 131072].

        // Arm thickness clamped to source's [0.05, 0.90] range of arm-spacing.
        constexpr int32_t kMinArmThicknessRaw = (SF16_ONE * 5) / 100;   // 0.05
        constexpr int32_t kMaxArmThicknessRaw = (SF16_ONE * 90) / 100;  // 0.90

        // Radial brightness gradient: weight = 0.6 + 0.4 * radius. Pre-computed q16 constants.
        constexpr uint32_t kGradientBaseQ16 = (SF16_ONE * 6) / 10;   // 0.6
        constexpr uint32_t kGradientSlopeQ16 = (SF16_ONE * 4) / 10;  // 0.4
    }

    struct SpiralPattern::State {
        uint8_t armCount;
        bool clockwise;
        Sf16Signal tightnessSignal;
        Sf16Signal armThicknessSignal;
        Sf16Signal rotationSpeedSignal;

        // Cached per-frame values (read by the per-pixel functor).
        int32_t tightnessTurnsRaw{0};   // q16 turns: tightness * radius_at_edge
        uint32_t halfArmRaw{0};         // q16 fraction of one arm-cycle (0..32768)
        uint16_t phaseRaw{0};           // f16 turns

        TimeMillis lastElapsedMs{0u};
        bool hasLastElapsed{false};

        State(
            uint8_t armCount,
            bool clockwise,
            Sf16Signal tightness,
            Sf16Signal armThickness,
            Sf16Signal rotationSpeed
        ) : armCount(armCount > 0 ? armCount : uint8_t(1)),
            clockwise(clockwise),
            tightnessSignal(std::move(tightness)),
            armThicknessSignal(std::move(armThickness)),
            rotationSpeedSignal(std::move(rotationSpeed)) {}
    };

    struct SpiralPattern::UVSpiralFunctor {
        const State *state;

        PatternNormU16 operator()(UV uv) const {
            // 1. Convert UV to polar: out.u = angle (f16 turns), out.v = radius (q16 [0, 1]).
            UV polar = cartesianToPolarUV(uv);
            uint16_t angleTurns = static_cast<uint16_t>(polar.u.raw());
            uint32_t radiusRaw = static_cast<uint32_t>(polar.v.raw());

            // 2. Spiral angle = angle - tightness*radius - phase (in f16 turn domain).
            uint32_t tightnessOffset = static_cast<uint32_t>(
                (static_cast<int64_t>(state->tightnessTurnsRaw) * static_cast<int64_t>(radiusRaw)) >> 16
            );
            uint16_t spiralAngle = static_cast<uint16_t>(
                angleTurns - static_cast<uint16_t>(tightnessOffset) - state->phaseRaw
            );

            // 3. Fold into one arm cycle. Multiplying by armCount and taking low 16 bits
            // performs an exact mod into a normalised [0, 65536) "arm-cycle" domain.
            uint16_t cyclePos = static_cast<uint16_t>(
                static_cast<uint32_t>(spiralAngle) * state->armCount
            );

            // 4. Distance to nearest arm centre (centre at 0 / 65536). Range [0, 32768].
            uint16_t distToCentre = (cyclePos <= 32768u)
                ? cyclePos
                : static_cast<uint16_t>(65536u - cyclePos);

            // 5. Outside the arm window -> zero intensity.
            if (distToCentre > state->halfArmRaw) {
                return PatternNormU16(0u);
            }

            // 6. Cosine falloff: t = distToCentre / halfArm in q16, intensity = 0.5 + 0.5*cos(pi*t).
            uint32_t t_q16 = (static_cast<uint32_t>(distToCentre) << 16) / state->halfArmRaw;
            uint16_t halfTurnAngle = static_cast<uint16_t>((t_q16 * HALF_TURN_U16) >> 16);
            int32_t cosRaw = raw(angleCosF16(f16(halfTurnAngle)));
            uint32_t intensity = static_cast<uint32_t>((cosRaw + SF16_ONE) >> 1);  // [0, 65535]

            // 7. Radial brightness gradient: weight = 0.6 + 0.4 * radius.
            uint32_t weight = kGradientBaseQ16 + ((kGradientSlopeQ16 * radiusRaw) >> 16);

            uint32_t out = (intensity * weight) >> 16;
            return PatternNormU16(static_cast<uint16_t>(out));
        }
    };

    SpiralPattern::SpiralPattern(
        uint8_t armCount,
        bool clockwise,
        Sf16Signal tightness,
        Sf16Signal armThickness,
        Sf16Signal rotationSpeed
    ) : state(std::make_shared<State>(
        armCount,
        clockwise,
        std::move(tightness),
        std::move(armThickness),
        std::move(rotationSpeed)
    )) {}

    void SpiralPattern::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void)progress;
        State &s = *state;

        // Delta-time in ms (clamped).
        TimeMillis dtMs = 0u;
        if (!s.hasLastElapsed) {
            s.lastElapsedMs = elapsedMs;
            s.hasLastElapsed = true;
        } else {
            dtMs = elapsedMs >= s.lastElapsedMs ? (elapsedMs - s.lastElapsedMs) : 0u;
            if (dtMs > MAX_DELTA_TIME_MS) dtMs = MAX_DELTA_TIME_MS;
            s.lastElapsedMs = elapsedMs;
        }

        // Sample signals (all in magnitude range [0, 1]).
        int32_t tightnessRaw = raw(s.tightnessSignal.sample(magnitudeRange(), elapsedMs));
        int32_t thicknessRaw = raw(s.armThicknessSignal.sample(magnitudeRange(), elapsedMs));
        int32_t rotationRaw = raw(s.rotationSpeedSignal.sample(magnitudeRange(), elapsedMs));

        // Cache: tightness in q16 turns (signal*2 turns at radius=1).
        s.tightnessTurnsRaw = tightnessRaw << 1;

        // Cache: halfArm in arm-cycle q16 domain. armSpacing*thickness/2 scaled by armCount
        // simplifies to thickness/2 because the cyclePos math in the functor already scales
        // by armCount. kMinArmThicknessRaw guarantees halfArmRaw >= 1.
        int32_t thickClamped = std::max(kMinArmThicknessRaw, std::min(kMaxArmThicknessRaw, thicknessRaw));
        s.halfArmRaw = static_cast<uint32_t>(thickClamped) >> 1;

        // Advance phase: max 1 turn/sec at signal=1.
        // delta_raw_q16_turns = rotation_raw * dt_ms / 1000.
        int32_t deltaTurns = static_cast<int32_t>(
            (static_cast<int64_t>(rotationRaw) * static_cast<int64_t>(dtMs)) / 1000
        );
        if (!s.clockwise) deltaTurns = -deltaTurns;
        s.phaseRaw = static_cast<uint16_t>(
            static_cast<uint32_t>(s.phaseRaw) + static_cast<uint32_t>(deltaTurns)
        );
    }

    UVMap SpiralPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        (void)context;
        return UVSpiralFunctor{state.get()};
    }
}
