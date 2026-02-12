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

        int32_t sampleSignedSpeedRaw(SQ0_16Signal &signal, TimeMillis elapsedMs) {
            return raw(signal.sample(signedUnitRange(), elapsedMs));
        }

        SPoint32 sampleNoisePair(int64_t sx_raw, int64_t sy_raw, int32_t timeOffsetRaw) {
            int64_t base = static_cast<int64_t>(NOISE_DOMAIN_OFFSET) << CARTESIAN_FRAC_BITS;
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
            return SPoint32{nx, ny};
        }

        SPoint32 sampleWarp(int64_t sx_raw, int64_t sy_raw, int32_t timeOffsetRaw, int32_t amp) {
            SPoint32 n = sampleNoisePair(sx_raw, sy_raw, timeOffsetRaw);
            int32_t dx = static_cast<int32_t>((static_cast<int64_t>(n.x) * amp) >> 16);
            int32_t dy = static_cast<int32_t>((static_cast<int64_t>(n.y) * amp) >> 16);
            return SPoint32{dx, dy};
        }

        SPoint32 sampleCurl(int64_t sx_raw, int64_t sy_raw, int32_t timeOffsetRaw, int32_t amp) {
            constexpr int32_t EPS = 1 << CARTESIAN_FRAC_BITS;
            int64_t base = static_cast<int64_t>(NOISE_DOMAIN_OFFSET) << CARTESIAN_FRAC_BITS;
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
            return SPoint32{dx, dy};
        }
    }

    struct DomainWarpTransform::MappedInputs {
        SQ0_16Signal phaseSpeedSignal;
        SQ0_16Signal amplitudeSignal;
        SQ0_16Signal warpScaleSignal;
        LinearRange<SQ24_8> warpScaleRange;
        SQ0_16Signal maxOffsetSignal;
        LinearRange<SQ24_8> maxOffsetRange;
        SQ0_16Signal flowDirectionSignal;
        PolarRange flowDirectionRange;
        SQ0_16Signal flowStrengthSignal;
        bool hasFlow{false};
    };

    DomainWarpTransform::MappedInputs DomainWarpTransform::makeInputs(
        SQ0_16Signal speed,
        SQ0_16Signal amplitude,
        SQ0_16Signal warpScale,
        SQ0_16Signal maxOffset,
        LinearRange<SQ24_8> warpScaleRange,
        LinearRange<SQ24_8> maxOffsetRange,
        SQ0_16Signal flowDirection,
        SQ0_16Signal flowStrength
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
        SQ0_16Signal amplitudeSignal;
        SQ0_16Signal warpScaleSignal;
        LinearRange<SQ24_8> warpScaleRange;
        SQ0_16Signal maxOffsetSignal;
        LinearRange<SQ24_8> maxOffsetRange;
        SQ24_8 warpScale = SQ24_8(0);
        SQ24_8 maxOffset = SQ24_8(0);
        uint8_t octaves;
        SQ0_16Signal flowDirectionSignal;
        PolarRange flowDirectionRange;
        SQ0_16Signal flowStrengthSignal;
        bool hasFlow{false};
        SPoint32 flowOffset{0, 0};
        int32_t timeOffsetRaw{0};
        int32_t amplitudeRaw{0};

        State(
            WarpType type,
            SQ0_16Signal phaseSpeedSignal,
            SQ0_16Signal amplitudeSignal,
            SQ0_16Signal warpScaleSignal,
            LinearRange<SQ24_8> warpScaleRange,
            SQ0_16Signal maxOffsetSignal,
            LinearRange<SQ24_8> maxOffsetRange,
            uint8_t octaves,
            SQ0_16Signal flowDirectionSignal,
            PolarRange flowDirectionRange,
            SQ0_16Signal flowStrengthSignal,
            bool hasFlow
        ) : type(type),
            phase(
                [speed = std::move(phaseSpeedSignal)](TimeMillis elapsedMs) mutable {
                    return SQ0_16(sampleSignedSpeedRaw(speed, elapsedMs));
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
        SQ0_16Signal speed,
        SQ0_16Signal amplitude,
        SQ0_16Signal warpScale,
        SQ0_16Signal maxOffset,
        LinearRange<SQ24_8> warpScaleRange,
        LinearRange<SQ24_8> maxOffsetRange
    ) : DomainWarpTransform(WarpType::Basic, std::move(speed), amplitude, warpScale, maxOffset, warpScaleRange, maxOffsetRange, 3) {
    }

    DomainWarpTransform::DomainWarpTransform(
        WarpType type,
        SQ0_16Signal speed,
        SQ0_16Signal amplitude,
        SQ0_16Signal warpScale,
        SQ0_16Signal maxOffset,
        LinearRange<SQ24_8> warpScaleRange,
        LinearRange<SQ24_8> maxOffsetRange,
        uint8_t octaves,
        SQ0_16Signal flowDirection,
        SQ0_16Signal flowStrength
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

    void DomainWarpTransform::advanceFrame(UQ0_16 progress, TimeMillis elapsedMs) {
        if (!context) {
            // Serial.println("DomainWarpTransform::advanceFrame context is null.");
        }

        UQ0_16 phase = state->phase.advance(elapsedMs);
        state->timeOffsetRaw = static_cast<int32_t>(raw(phase)) << CARTESIAN_FRAC_BITS;

        state->warpScale = state->warpScaleSignal.sample(state->warpScaleRange, elapsedMs);
        state->maxOffset = state->maxOffsetSignal.sample(state->maxOffsetRange, elapsedMs);

        int32_t maxOffsetRaw = raw(state->maxOffset);
        uint32_t ampT = static_cast<uint32_t>(raw(state->amplitudeSignal.sample(unitRange(), elapsedMs)));
        int64_t ampScaled = (static_cast<int64_t>(maxOffsetRaw) * static_cast<int64_t>(ampT)) >> 16;
        state->amplitudeRaw = static_cast<int32_t>(ampScaled);

        if (state->type == WarpType::Directional && state->hasFlow && state->flowDirectionSignal && state->flowStrengthSignal) {
            UQ0_16 dir = state->flowDirectionSignal.sample(state->flowDirectionRange, elapsedMs);
            uint32_t flowT = static_cast<uint32_t>(raw(state->flowStrengthSignal.sample(unitRange(), elapsedMs)));
            int32_t strength = static_cast<int32_t>(
                (static_cast<int64_t>(maxOffsetRaw) * static_cast<int64_t>(flowT)) >> 16
            );
            SQ0_16 cos_q0_16 = angleCosQ0_16(dir);
            SQ0_16 sin_q0_16 = angleSinQ0_16(dir);
            int64_t dx = static_cast<int64_t>(strength) * static_cast<int64_t>(raw(cos_q0_16));
            int64_t dy = static_cast<int64_t>(strength) * static_cast<int64_t>(raw(sin_q0_16));
            dx = (dx >= 0) ? (dx + (1LL << 15)) : (dx - (1LL << 15));
            dy = (dy >= 0) ? (dy + (1LL << 15)) : (dy - (1LL << 15));
            dx >>= 16;
            dy >>= 16;
            state->flowOffset = SPoint32{static_cast<int32_t>(dx), static_cast<int32_t>(dy)};
        } else {
            state->flowOffset = SPoint32{0, 0};
        }
    }

    UVMap DomainWarpTransform::operator()(const UVMap &layer) const {
        return [state = this->state, layer](UV uv) {
            SQ24_8 cx = CartesianMaths::from_uv(uv.u);
            SQ24_8 cy = CartesianMaths::from_uv(uv.v);

            SQ24_8 sx = CartesianMaths::mul(cx, state->warpScale);
            SQ24_8 sy = CartesianMaths::mul(cy, state->warpScale);

            SPoint32 warp{0, 0};
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
                        SPoint32 step = sampleWarp(ox, oy, time_offset, amp);
                        warp.x += step.x;
                        warp.y += step.y;
                        amp >>= 1;
                    }
                    break;
                }
                case WarpType::Nested: {
                    SPoint32 first = sampleWarp(sx_raw, sy_raw, time_offset, state->amplitudeRaw);
                    int64_t wx = sx_raw + first.x;
                    int64_t wy = sy_raw + first.y;
                    int32_t amp = state->amplitudeRaw >> 1;
                    SPoint32 second = sampleWarp(wx << 1, wy << 1, time_offset, amp);
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

                    SPoint32 polarNoise = sampleNoisePair(raw(polar_uv.u), raw(polar_uv.v), time_offset);

                    int32_t uv_amp = state->amplitudeRaw << 8;
                    int32_t angle_delta = static_cast<int32_t>((static_cast<int64_t>(polarNoise.x) * uv_amp) >> 16);
                    int32_t radius_delta = static_cast<int32_t>((static_cast<int64_t>(polarNoise.y) * uv_amp) >> 16);

                    polar_uv.u = SQ16_16(raw(polar_uv.u) + angle_delta);
                    polar_uv.v = SQ16_16(raw(polar_uv.v) + radius_delta);

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
                SQ16_16(raw(uv.u) + (warp.x << 8)),
                SQ16_16(raw(uv.v) + (warp.y << 8))
            );
            return layer(warped_uv);
        };
    }
}
