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

#include "DomainWarpTransform.h"
#include "renderer/pipeline/maths/AngleMaths.h"
#include "renderer/pipeline/maths/CartesianMaths.h"
#include "renderer/pipeline/maths/NoiseMaths.h"
#include "renderer/pipeline/maths/PolarMaths.h"
#include "renderer/pipeline/signals/ranges/PolarRange.h"
#include "renderer/pipeline/signals/ranges/LinearRange.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/signals/accumulators/SignalAccumulators.h"
#include "renderer/pipeline/signals/accumulators/Accumulators.h"
#include <utility>
#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif

namespace PolarShader {
    namespace {
        constexpr uint32_t WARP_SEED_X = 0x9E3779B9u;
        constexpr uint32_t WARP_SEED_Y = 0x7F4A7C15u;
        constexpr uint32_t WARP_SEED_Z = 0xB5297A4Du;
        constexpr uint32_t WARP_SEED_W = 0x68E31DA4u;

        int32_t sampleSignedSpeedRaw(Sf16Signal &signal, TimeMillis elapsedMs) {
            return raw(signal.sample(signedUnitRange(), elapsedMs));
        }

        v32 sampleNoisePair(int64_t sx_raw, int64_t sy_raw, int32_t timeOffsetRaw) {
            int64_t base = static_cast<int64_t>(NOISE_DOMAIN_OFFSET) << R8_FRAC_BITS;
            uint32_t ux = static_cast<uint32_t>(sx_raw + base);
            uint32_t uy = static_cast<uint32_t>(sy_raw + base);
            uint32_t uz = static_cast<uint32_t>(static_cast<int64_t>(timeOffsetRaw) + base);

            PatternNormU16 n0 = noiseNormaliseU16(sampleNoiseTrilinear(
                ux + WARP_SEED_X,
                uy + WARP_SEED_Y,
                uz + WARP_SEED_Z
            ));
            PatternNormU16 n1 = noiseNormaliseU16(sampleNoiseTrilinear(
                ux + WARP_SEED_Z,
                uy + WARP_SEED_W,
                uz + WARP_SEED_X
            ));

            int32_t nx = static_cast<int32_t>(raw(n0)) - static_cast<int32_t>(U16_HALF);
            int32_t ny = static_cast<int32_t>(raw(n1)) - static_cast<int32_t>(U16_HALF);
            return v32{nx, ny};
        }

        v32 sampleWarp(int64_t sx_raw, int64_t sy_raw, int32_t timeOffsetRaw, int32_t amp) {
            v32 n = sampleNoisePair(sx_raw, sy_raw, timeOffsetRaw);
            int32_t dx = static_cast<int32_t>((static_cast<int64_t>(n.x) * amp) >> 16);
            int32_t dy = static_cast<int32_t>((static_cast<int64_t>(n.y) * amp) >> 16);
            return v32{dx, dy};
        }

        v32 sampleCurl(int64_t sx_raw, int64_t sy_raw, int32_t timeOffsetRaw, int32_t amp) {
            constexpr int32_t EPS = 1 << R8_FRAC_BITS;
            int64_t base = static_cast<int64_t>(NOISE_DOMAIN_OFFSET) << R8_FRAC_BITS;
            uint32_t ux = static_cast<uint32_t>(sx_raw + base);
            uint32_t uy = static_cast<uint32_t>(sy_raw + base);
            uint32_t uz = static_cast<uint32_t>(static_cast<int64_t>(timeOffsetRaw) + base);

            auto n_x1 = noiseNormaliseU16(sampleNoiseTrilinear(ux + EPS, uy, uz));
            auto n_x0 = noiseNormaliseU16(sampleNoiseTrilinear(ux - EPS, uy, uz));
            auto n_y1 = noiseNormaliseU16(sampleNoiseTrilinear(ux, uy + EPS, uz));
            auto n_y0 = noiseNormaliseU16(sampleNoiseTrilinear(ux, uy - EPS, uz));

            int32_t dnx = static_cast<int32_t>(raw(n_x1)) - static_cast<int32_t>(raw(n_x0));
            int32_t dny = static_cast<int32_t>(raw(n_y1)) - static_cast<int32_t>(raw(n_y0));

            int32_t dx = static_cast<int32_t>((static_cast<int64_t>(dny) * amp) >> 16);
            int32_t dy = static_cast<int32_t>((static_cast<int64_t>(-dnx) * amp) >> 16);
            return v32{dx, dy};
        }
    }

    struct DomainWarpTransform::MappedInputs {
        Sf16Signal phaseSpeedSignal;
        Sf16Signal amplitudeSignal;
        Sf16Signal warpScaleSignal;
        LinearRange<sr8> warpScaleRange;
        Sf16Signal maxOffsetSignal;
        LinearRange<sr8> maxOffsetRange;
        Sf16Signal flowDirectionSignal;
        PolarRange flowDirectionRange;
        Sf16Signal flowStrengthSignal;
        bool hasFlow{false};
    };

    DomainWarpTransform::MappedInputs DomainWarpTransform::makeInputs(
        Sf16Signal speed,
        Sf16Signal amplitude,
        Sf16Signal warpScale,
        Sf16Signal maxOffset,
        LinearRange<sr8> warpScaleRange,
        LinearRange<sr8> maxOffsetRange,
        Sf16Signal flowDirection,
        Sf16Signal flowStrength
    ) {
        bool hasFlow = static_cast<bool>(flowDirection) && static_cast<bool>(flowStrength);

        return DomainWarpTransform::MappedInputs{
            std::move(speed),
            std::move(amplitude),
            std::move(warpScale),
            std::move(warpScaleRange),
            std::move(maxOffset),
            std::move(maxOffsetRange),
            std::move(flowDirection),
            PolarRange(),
            std::move(flowStrength),
            hasFlow
        };
    }

    struct DomainWarpTransform::State {
        WarpType type;
        PhaseAccumulator phase;
        Sf16Signal amplitudeSignal;
        Sf16Signal warpScaleSignal;
        LinearRange<sr8> warpScaleRange;
        Sf16Signal maxOffsetSignal;
        LinearRange<sr8> maxOffsetRange;
        sr8 warpScale = sr8(0);
        sr8 maxOffset = sr8(0);
        uint8_t octaves;
        Sf16Signal flowDirectionSignal;
        PolarRange flowDirectionRange;
        Sf16Signal flowStrengthSignal;
        bool hasFlow{false};
        v32 flowOffset{0, 0};
        int32_t timeOffsetRaw{0};
        int32_t amplitudeRaw{0};

        State(
            WarpType type,
            Sf16Signal phaseSpeedSignal,
            Sf16Signal amplitudeSignal,
            Sf16Signal warpScaleSignal,
            LinearRange<sr8> warpScaleRange,
            Sf16Signal maxOffsetSignal,
            LinearRange<sr8> maxOffsetRange,
            uint8_t octaves,
            Sf16Signal flowDirectionSignal,
            PolarRange flowDirectionRange,
            Sf16Signal flowStrengthSignal,
            bool hasFlow
        ) : type(type),
            phase(
                [speed = std::move(phaseSpeedSignal)](TimeMillis elapsedMs) mutable {
                    return sf16(sampleSignedSpeedRaw(speed, elapsedMs));
                }
            ),
            amplitudeSignal(std::move(amplitudeSignal)),
            warpScaleSignal(std::move(warpScaleSignal)),
            warpScaleRange(std::move(warpScaleRange)),
            maxOffsetSignal(std::move(maxOffsetSignal)),
            maxOffsetRange(std::move(maxOffsetRange)),
            octaves(octaves),
            flowDirectionSignal(std::move(flowDirectionSignal)),
            flowDirectionRange(std::move(flowDirectionRange)),
            flowStrengthSignal(std::move(flowStrengthSignal)),
            hasFlow(hasFlow) {
        }
    };

    DomainWarpTransform::DomainWarpTransform(
        Sf16Signal speed,
        Sf16Signal amplitude,
        Sf16Signal warpScale,
        Sf16Signal maxOffset,
        LinearRange<sr8> warpScaleRange,
        LinearRange<sr8> maxOffsetRange
    ) : DomainWarpTransform(WarpType::Basic, std::move(speed), amplitude, warpScale, maxOffset, warpScaleRange, maxOffsetRange, 3) {
    }

    DomainWarpTransform::DomainWarpTransform(
        WarpType type,
        Sf16Signal speed,
        Sf16Signal amplitude,
        Sf16Signal warpScale,
        Sf16Signal maxOffset,
        LinearRange<sr8> warpScaleRange,
        LinearRange<sr8> maxOffsetRange,
        uint8_t octaves,
        Sf16Signal flowDirection,
        Sf16Signal flowStrength
    ) {
        auto inputs = makeInputs(std::move(speed), amplitude, warpScale, maxOffset, warpScaleRange, maxOffsetRange, flowDirection, flowStrength);
        state = std::make_shared<State>(
            type,
            std::move(inputs.phaseSpeedSignal),
            std::move(inputs.amplitudeSignal),
            std::move(inputs.warpScaleSignal),
            std::move(inputs.warpScaleRange),
            std::move(inputs.maxOffsetSignal),
            std::move(inputs.maxOffsetRange),
            octaves,
            std::move(inputs.flowDirectionSignal),
            std::move(inputs.flowDirectionRange),
            std::move(inputs.flowStrengthSignal),
            inputs.hasFlow
        );
    }

    void DomainWarpTransform::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        if (!context) {
            // Serial.println("DomainWarpTransform::advanceFrame context is null.");
        }

        f16 phase = state->phase.advance(elapsedMs);
        state->timeOffsetRaw = static_cast<int32_t>(raw(phase)) << R8_FRAC_BITS;

        state->warpScale = state->warpScaleSignal.sample(state->warpScaleRange, elapsedMs);
        state->maxOffset = state->maxOffsetSignal.sample(state->maxOffsetRange, elapsedMs);

        int32_t maxOffsetRaw = raw(state->maxOffset);
        uint32_t ampT = static_cast<uint32_t>(raw(state->amplitudeSignal.sample(unitRange(), elapsedMs)));
        int64_t ampScaled = (static_cast<int64_t>(maxOffsetRaw) * static_cast<int64_t>(ampT)) >> 16;
        state->amplitudeRaw = static_cast<int32_t>(ampScaled);

        if (state->type == WarpType::Directional && state->hasFlow && state->flowDirectionSignal && state->flowStrengthSignal) {
            f16 dir = state->flowDirectionSignal.sample(state->flowDirectionRange, elapsedMs);
            uint32_t flowT = static_cast<uint32_t>(raw(state->flowStrengthSignal.sample(unitRange(), elapsedMs)));
            int32_t strength = static_cast<int32_t>(
                (static_cast<int64_t>(maxOffsetRaw) * static_cast<int64_t>(flowT)) >> 16
            );
            sf16 cos_q0_16 = angleCosF16(dir);
            sf16 sin_q0_16 = angleSinF16(dir);
            int64_t dx = static_cast<int64_t>(strength) * static_cast<int64_t>(raw(cos_q0_16));
            int64_t dy = static_cast<int64_t>(strength) * static_cast<int64_t>(raw(sin_q0_16));
            dx = (dx >= 0) ? (dx + (1LL << 15)) : (dx - (1LL << 15));
            dy = (dy >= 0) ? (dy + (1LL << 15)) : (dy - (1LL << 15));
            dx >>= 16;
            dy >>= 16;
            state->flowOffset = v32{static_cast<int32_t>(dx), static_cast<int32_t>(dy)};
        } else {
            state->flowOffset = v32{0, 0};
        }
    }

    UVMap DomainWarpTransform::operator()(const UVMap &layer) const {
        return [state = this->state, layer](UV uv) {
            sr8 cx = CartesianMaths::from_uv(uv.u);
            sr8 cy = CartesianMaths::from_uv(uv.v);

            sr8 sx = CartesianMaths::mul(cx, state->warpScale);
            sr8 sy = CartesianMaths::mul(cy, state->warpScale);

            v32 warp{0, 0};
            int64_t sx_raw = raw(sx);
            int64_t sy_raw = raw(sy);
            int32_t time_offset = state->timeOffsetRaw;

            switch (state->type) {
                case WarpType::FBM: {
                    int32_t amp = state->amplitudeRaw;
                    uint8_t octaves = state->octaves ? state->octaves : 1;
                    for (uint8_t o = 0; o < octaves && amp > 0; o++) {
                        int64_t ox = sx_raw << o;
                        int64_t oy = sy_raw << o;
                        v32 step = sampleWarp(ox, oy, time_offset, amp);
                        warp.x += step.x;
                        warp.y += step.y;
                        amp >>= 1;
                    }
                    break;
                }
                case WarpType::Nested: {
                    v32 first = sampleWarp(sx_raw, sy_raw, time_offset, state->amplitudeRaw);
                    int64_t wx = sx_raw + first.x;
                    int64_t wy = sy_raw + first.y;
                    int32_t amp = state->amplitudeRaw >> 1;
                    v32 second = sampleWarp(wx << 1, wy << 1, time_offset, amp);
                    warp.x = first.x + second.x;
                    warp.y = first.y + second.y;
                    break;
                }
                case WarpType::Curl: {
                    warp = sampleCurl(sx_raw, sy_raw, time_offset, state->amplitudeRaw);
                    break;
                }
                case WarpType::Polar: {
                    UV current_uv(CartesianMaths::to_uv(sx), CartesianMaths::to_uv(sy));
                    UV polar_uv = cartesianToPolarUV(current_uv);

                    v32 polarNoise = sampleNoisePair(raw(polar_uv.u), raw(polar_uv.v), time_offset);

                    int32_t uv_amp = state->amplitudeRaw << 8;
                    int32_t angle_delta = static_cast<int32_t>((static_cast<int64_t>(polarNoise.x) * uv_amp) >> 16);
                    int32_t radius_delta = static_cast<int32_t>((static_cast<int64_t>(polarNoise.y) * uv_amp) >> 16);

                    polar_uv.u = sr16(raw(polar_uv.u) + angle_delta);
                    polar_uv.v = sr16(raw(polar_uv.v) + radius_delta);

                    return layer(polarToCartesianUV(polar_uv));
                }
                case WarpType::Directional: {
                    warp = sampleWarp(sx_raw, sy_raw, time_offset, state->amplitudeRaw);
                    warp.x += state->flowOffset.x;
                    warp.y += state->flowOffset.y;
                    break;
                }
                case WarpType::Basic:
                default: {
                    warp = sampleWarp(sx_raw, sy_raw, time_offset, state->amplitudeRaw);
                    break;
                }
            }

            UV warped_uv(
                sr16(raw(uv.u) + (warp.x << 8)),
                sr16(raw(uv.v) + (warp.y << 8))
            );
            return layer(warped_uv);
        };
    }
}
