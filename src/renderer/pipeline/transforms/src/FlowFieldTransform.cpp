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

#include "renderer/pipeline/transforms/FlowFieldTransform.h"
#include "renderer/pipeline/maths/CartesianMaths.h"
#include "renderer/pipeline/maths/NoiseMaths.h"
#include "renderer/pipeline/signals/accumulators/Accumulators.h"
#include "renderer/pipeline/signals/Signals.h"
#include <array>

namespace PolarShader {
    namespace {
        constexpr uint32_t FLOW_SEED_X = 0xA511E9B3u;
        constexpr uint32_t FLOW_SEED_Y = 0x63D83595u;
        constexpr uint32_t FLOW_SEED_Z = 0xC2B2AE3Du;
        constexpr int32_t FLOW_EPS = 1 << 6;
        constexpr uint8_t FLOW_GRID_W = 16;
        constexpr uint8_t FLOW_GRID_H = 16;
        constexpr uint8_t FLOW_TRACE_STEPS = 4;

        int32_t sampleSignedSpeedRaw(Sf16Signal &signal, TimeMillis elapsedMs) {
            return raw(signal.sample(bipolarRange(), elapsedMs));
        }

        v32 sampleCurlVector(
            int64_t xRaw,
            int64_t yRaw,
            fl::s24x8 fieldScale,
            int32_t timeOffsetRaw,
            int32_t strengthRaw
        ) {
            const int64_t base = static_cast<int64_t>(NOISE_DOMAIN_OFFSET) << 8;
            const int64_t sxRaw = xRaw * static_cast<int64_t>(raw(fieldScale));
            const int64_t syRaw = yRaw * static_cast<int64_t>(raw(fieldScale));
            const uint32_t ux = static_cast<uint32_t>(sxRaw + base);
            const uint32_t uy = static_cast<uint32_t>(syRaw + base);
            const uint32_t uz = static_cast<uint32_t>(static_cast<int64_t>(timeOffsetRaw) + base);

            PatternNormU16 nX1 = noiseNormaliseU16(sampleNoiseTrilinear(
                ux + FLOW_EPS + FLOW_SEED_X,
                uy + FLOW_SEED_Y,
                uz + FLOW_SEED_Z
            ));
            PatternNormU16 nX0 = noiseNormaliseU16(sampleNoiseTrilinear(
                ux - FLOW_EPS + FLOW_SEED_X,
                uy + FLOW_SEED_Y,
                uz + FLOW_SEED_Z
            ));
            PatternNormU16 nY1 = noiseNormaliseU16(sampleNoiseTrilinear(
                ux + FLOW_SEED_X,
                uy + FLOW_EPS + FLOW_SEED_Y,
                uz + FLOW_SEED_Z
            ));
            PatternNormU16 nY0 = noiseNormaliseU16(sampleNoiseTrilinear(
                ux + FLOW_SEED_X,
                uy - FLOW_EPS + FLOW_SEED_Y,
                uz + FLOW_SEED_Z
            ));

            const int32_t dnx = static_cast<int32_t>(raw(nX1)) - static_cast<int32_t>(raw(nX0));
            const int32_t dny = static_cast<int32_t>(raw(nY1)) - static_cast<int32_t>(raw(nY0));

            return v32{
                static_cast<int32_t>((static_cast<int64_t>(dny) * strengthRaw) >> 16),
                static_cast<int32_t>((static_cast<int64_t>(-dnx) * strengthRaw) >> 16)
            };
        }

        uint32_t clampUvRaw(int32_t rawUv) {
            if (rawUv <= 0) return 0u;
            if (rawUv >= 0x00010000) return 0x00010000u;
            return static_cast<uint32_t>(rawUv);
        }

        int32_t lerpRaw(int32_t a, int32_t b, uint32_t t) {
            return static_cast<int32_t>(
                static_cast<int64_t>(a) +
                (((static_cast<int64_t>(b) - static_cast<int64_t>(a)) * static_cast<int64_t>(t)) >> 16)
            );
        }

        v32 traceCurlDisplacement(
            int32_t startXRaw,
            int32_t startYRaw,
            fl::s24x8 fieldScale,
            int32_t timeOffsetRaw,
            int32_t strengthRaw
        ) {
            if (strengthRaw == 0) {
                return v32{0, 0};
            }

            int32_t stepStrength = strengthRaw / FLOW_TRACE_STEPS;
            if (stepStrength <= 0) {
                stepStrength = strengthRaw;
            }

            int64_t currentX = startXRaw;
            int64_t currentY = startYRaw;
            v32 total{0, 0};
            for (uint8_t i = 0; i < FLOW_TRACE_STEPS; ++i) {
                v32 step = sampleCurlVector(currentX, currentY, fieldScale, timeOffsetRaw, stepStrength);
                total.x += step.x;
                total.y += step.y;
                currentX += step.x;
                currentY += step.y;
            }
            return total;
        }

    }

    struct FlowFieldTransform::State {
        PhaseAccumulator phase;
        Sf16Signal flowStrengthSignal;
        Sf16Signal fieldScaleSignal;
        MagnitudeRange<fl::s24x8> fieldScaleRange;
        Sf16Signal maxOffsetSignal;
        MagnitudeRange<fl::s24x8> maxOffsetRange;
        fl::s24x8 fieldScale = fl::s24x8::from_raw(0);
        fl::s24x8 maxOffset = fl::s24x8::from_raw(0);
        int32_t timeOffsetRaw{0};
        int32_t flowStrengthRaw{0};
        std::array<v32, FLOW_GRID_W * FLOW_GRID_H> grid{};

        State(
            Sf16Signal phaseSpeedSignal,
            Sf16Signal flowStrengthSignal,
            Sf16Signal fieldScaleSignal,
            MagnitudeRange<fl::s24x8> fieldScaleRange,
            Sf16Signal maxOffsetSignal,
            MagnitudeRange<fl::s24x8> maxOffsetRange
        ) : phase(
                [speed = std::move(phaseSpeedSignal)](TimeMillis elapsedMs) mutable {
                    return sf16(sampleSignedSpeedRaw(speed, elapsedMs));
                }
            ),
            flowStrengthSignal(std::move(flowStrengthSignal)),
            fieldScaleSignal(std::move(fieldScaleSignal)),
            fieldScaleRange(std::move(fieldScaleRange)),
            maxOffsetSignal(std::move(maxOffsetSignal)),
            maxOffsetRange(std::move(maxOffsetRange)) {
        }
    };

    FlowFieldTransform::FlowFieldTransform(
        Sf16Signal phaseSpeed,
        Sf16Signal flowStrength,
        Sf16Signal fieldScale,
        Sf16Signal maxOffset,
        MagnitudeRange<fl::s24x8> fieldScaleRange,
        MagnitudeRange<fl::s24x8> maxOffsetRange
    ) : state(std::make_shared<State>(
        std::move(phaseSpeed),
        std::move(flowStrength),
        std::move(fieldScale),
        std::move(fieldScaleRange),
        std::move(maxOffset),
        std::move(maxOffsetRange)
    )) {
    }

    void FlowFieldTransform::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void)progress;
        f16 phase = state->phase.advance(elapsedMs);
        state->timeOffsetRaw = static_cast<int32_t>(raw(phase)) << 8;
        state->fieldScale = state->fieldScaleSignal.sample(state->fieldScaleRange, elapsedMs);
        state->maxOffset = state->maxOffsetSignal.sample(state->maxOffsetRange, elapsedMs);

        int32_t maxOffsetRaw = state->maxOffset.raw();
        uint32_t strengthT = static_cast<uint32_t>(raw(state->flowStrengthSignal.sample(magnitudeRange(), elapsedMs)));
        state->flowStrengthRaw = static_cast<int32_t>(
            (static_cast<int64_t>(maxOffsetRaw) * static_cast<int64_t>(strengthT)) >> 16
        );

        for (uint8_t gy = 0; gy < FLOW_GRID_H; ++gy) {
            uint32_t vRaw = (static_cast<uint32_t>(gy) * 0x00010000u) / (FLOW_GRID_H - 1u);
            int32_t yRaw = static_cast<int32_t>(vRaw >> 8);
            for (uint8_t gx = 0; gx < FLOW_GRID_W; ++gx) {
                uint32_t uRaw = (static_cast<uint32_t>(gx) * 0x00010000u) / (FLOW_GRID_W - 1u);
                int32_t xRaw = static_cast<int32_t>(uRaw >> 8);
                state->grid[gy * FLOW_GRID_W + gx] = traceCurlDisplacement(
                    xRaw,
                    yRaw,
                    state->fieldScale,
                    state->timeOffsetRaw,
                    state->flowStrengthRaw
                );
            }
        }
    }

    UVMap FlowFieldTransform::operator()(const UVMap &layer) const {
        return [state = this->state, layer](UV uv) {
            uint32_t uRaw = clampUvRaw(uv.u.raw());
            uint32_t vRaw = clampUvRaw(uv.v.raw());
            uint32_t scaledU = uRaw * (FLOW_GRID_W - 1u);
            uint32_t scaledV = vRaw * (FLOW_GRID_H - 1u);
            uint32_t gx0 = scaledU >> 16;
            uint32_t gy0 = scaledV >> 16;
            uint32_t tx = scaledU & 0xFFFFu;
            uint32_t ty = scaledV & 0xFFFFu;
            uint32_t gx1 = (gx0 + 1u < FLOW_GRID_W) ? (gx0 + 1u) : gx0;
            uint32_t gy1 = (gy0 + 1u < FLOW_GRID_H) ? (gy0 + 1u) : gy0;

            const v32 &f00 = state->grid[gy0 * FLOW_GRID_W + gx0];
            const v32 &f10 = state->grid[gy0 * FLOW_GRID_W + gx1];
            const v32 &f01 = state->grid[gy1 * FLOW_GRID_W + gx0];
            const v32 &f11 = state->grid[gy1 * FLOW_GRID_W + gx1];

            int32_t topX = lerpRaw(f00.x, f10.x, tx);
            int32_t topY = lerpRaw(f00.y, f10.y, tx);
            int32_t bottomX = lerpRaw(f01.x, f11.x, tx);
            int32_t bottomY = lerpRaw(f01.y, f11.y, tx);
            v32 flow{
                lerpRaw(topX, bottomX, ty),
                lerpRaw(topY, bottomY, ty)
            };

            UV flowedUv(
                fl::s16x16::from_raw(uv.u.raw() + (flow.x << 8)),
                fl::s16x16::from_raw(uv.v.raw() + (flow.y << 8))
            );
            return layer(flowedUv);
        };
    }
}
