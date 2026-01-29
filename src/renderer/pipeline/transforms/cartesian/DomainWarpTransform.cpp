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
#include "renderer/pipeline/ranges/PolarRange.h"
#include "renderer/pipeline/ranges/SFracRange.h"
#include "renderer/pipeline/ranges/ScalarRange.h"
#include "renderer/pipeline/units/UnitConstants.h"
#include <cstdint>
#include <utility>
#include <Arduino.h>

namespace PolarShader {
    namespace {
        constexpr uint32_t WARP_SEED_X = 0x9E3779B9u;
        constexpr uint32_t WARP_SEED_Y = 0x7F4A7C15u;
        constexpr uint32_t WARP_SEED_Z = 0xB5297A4Du;
        constexpr uint32_t WARP_SEED_W = 0x68E31DA4u;
    }

    struct DomainWarpTransform::MappedInputs {
        MappedSignal<SFracQ0_16> phaseSpeedSignal;
        MappedSignal<int32_t> amplitudeSignal;
        MappedSignal<FracQ0_16> flowDirectionSignal;
        MappedSignal<int32_t> flowStrengthSignal;
    };

    struct DomainWarpTransform::State {
        WarpType type;
        PhaseAccumulator phase;
        MappedSignal<int32_t> amplitudeSignal;
        CartQ24_8 warpScale;
        CartQ24_8 maxOffset;
        uint8_t octaves;
        MappedSignal<FracQ0_16> flowDirectionSignal;
        MappedSignal<int32_t> flowStrengthSignal;
        SPoint32 flowOffset{0, 0};
        int32_t timeOffsetRaw{0};
        int32_t amplitudeRaw{0};

        State(
            WarpType type,
            MappedSignal<SFracQ0_16> phaseSpeed,
            MappedSignal<int32_t> amplitude,
            CartQ24_8 scale,
            CartQ24_8 offset,
            uint8_t octaves,
            MappedSignal<FracQ0_16> flowDirection,
            MappedSignal<int32_t> flowStrength
        ) : type(type),
            phase(std::move(phaseSpeed)),
            amplitudeSignal(std::move(amplitude)),
            warpScale(scale),
            maxOffset(offset),
            octaves(octaves),
            flowDirectionSignal(std::move(flowDirection)),
            flowStrengthSignal(std::move(flowStrength)) {
        }
    };

    DomainWarpTransform::DomainWarpTransform(
        WarpType type,
        MappedSignal<SFracQ0_16> phaseSpeed,
        MappedSignal<int32_t> amplitude,
        CartQ24_8 warpScale,
        CartQ24_8 maxOffset,
        uint8_t octaves,
        MappedSignal<FracQ0_16> flowDirection,
        MappedSignal<int32_t> flowStrength
    ) : state(std::make_shared<State>(
        type,
        std::move(phaseSpeed),
        std::move(amplitude),
        warpScale,
        maxOffset,
        octaves,
        std::move(flowDirection),
        std::move(flowStrength)
    )) {
    }

    DomainWarpTransform::DomainWarpTransform(
        WarpType type,
        CartQ24_8 warpScale,
        CartQ24_8 maxOffset,
        uint8_t octaves,
        MappedInputs inputs
    ) : DomainWarpTransform(
        type,
        std::move(inputs.phaseSpeedSignal),
        std::move(inputs.amplitudeSignal),
        warpScale,
        maxOffset,
        octaves,
        std::move(inputs.flowDirectionSignal),
        std::move(inputs.flowStrengthSignal)
    ) {
    }

    DomainWarpTransform::MappedInputs DomainWarpTransform::makeInputs(
        SFracQ0_16Signal phaseSpeed,
        SFracQ0_16Signal amplitude,
        CartQ24_8 maxOffset
    ) {
        return makeInputsInternal(
            std::move(phaseSpeed),
            std::move(amplitude),
            maxOffset,
            SFracQ0_16Signal(),
            SFracQ0_16Signal()
        );
    }

    DomainWarpTransform::MappedInputs DomainWarpTransform::makeDirectionalInputs(
        SFracQ0_16Signal phaseSpeed,
        SFracQ0_16Signal amplitude,
        CartQ24_8 maxOffset,
        SFracQ0_16Signal flowDirection,
        SFracQ0_16Signal flowStrength
    ) {
        return makeInputsInternal(
            std::move(phaseSpeed),
            std::move(amplitude),
            maxOffset,
            std::move(flowDirection),
            std::move(flowStrength)
        );
    }

    DomainWarpTransform::MappedInputs DomainWarpTransform::makeInputsInternal(
        SFracQ0_16Signal phaseSpeed,
        SFracQ0_16Signal amplitude,
        CartQ24_8 maxOffset,
        SFracQ0_16Signal flowDirection,
        SFracQ0_16Signal flowStrength
    ) {
        SFracRange phaseRange(SFracQ0_16(0), SFracQ0_16(Q0_16_ONE));
        ScalarRange amplitudeRange(0, raw(maxOffset) > 0 ? raw(maxOffset) : 0);
        PolarRange flowDirectionRange;
        ScalarRange flowStrengthRange(0, raw(maxOffset) > 0 ? raw(maxOffset) : 0);
        bool hasFlow = flowDirection && flowStrength;

        return MappedInputs{
            phaseRange.mapSignal(std::move(phaseSpeed)),
            amplitudeRange.mapSignal(std::move(amplitude)),
            hasFlow ? flowDirectionRange.mapSignal(std::move(flowDirection)) : MappedSignal<FracQ0_16>(),
            hasFlow ? flowStrengthRange.mapSignal(std::move(flowStrength)) : MappedSignal<int32_t>()
        };
    }

    DomainWarpTransform::DomainWarpTransform(
        SFracQ0_16Signal phaseSpeed,
        SFracQ0_16Signal amplitude,
        CartQ24_8 warpScale,
        CartQ24_8 maxOffset
    ) : DomainWarpTransform(
        WarpType::Basic,
        warpScale,
        maxOffset,
        3,
        makeInputs(std::move(phaseSpeed), std::move(amplitude), maxOffset)
    ) {
    }

    DomainWarpTransform::DomainWarpTransform(
        WarpType type,
        SFracQ0_16Signal phaseSpeed,
        SFracQ0_16Signal amplitude,
        CartQ24_8 warpScale,
        CartQ24_8 maxOffset,
        uint8_t octaves,
        SFracQ0_16Signal flowDirection,
        SFracQ0_16Signal flowStrength
    ) : DomainWarpTransform(
        type,
        warpScale,
        maxOffset,
        octaves,
        makeDirectionalInputs(
            std::move(phaseSpeed),
            std::move(amplitude),
            maxOffset,
            std::move(flowDirection),
            std::move(flowStrength)
        )
    ) {
    }

    void DomainWarpTransform::advanceFrame(TimeMillis timeInMillis) {
        if (!context) {
            Serial.println("DomainWarpTransform::advanceFrame context is null.");
        }

        SFracQ0_16 phase = state->phase.advance(timeInMillis);
        state->timeOffsetRaw = raw(phase) << CARTESIAN_FRAC_BITS;

        state->amplitudeRaw = state->amplitudeSignal(timeInMillis).get();

        if (state->type == WarpType::Directional && state->flowDirectionSignal && state->flowStrengthSignal) {
            FracQ0_16 dir = state->flowDirectionSignal(timeInMillis).get();
            int32_t strength = state->flowStrengthSignal(timeInMillis).get();
            TrigQ0_16 cos_q0_16 = angleCosQ0_16(dir);
            TrigQ0_16 sin_q0_16 = angleSinQ0_16(dir);
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

    CartesianLayer DomainWarpTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](CartQ24_8 x, CartQ24_8 y) {
            CartQ24_8 sx = CartesianMaths::mul(x, state->warpScale);
            CartQ24_8 sy = CartesianMaths::mul(y, state->warpScale);

            int64_t base = static_cast<int64_t>(NOISE_DOMAIN_OFFSET) << CARTESIAN_FRAC_BITS;

            auto sampleNoisePair = [&](int64_t sx_raw, int64_t sy_raw) {
                uint32_t ux = static_cast<uint32_t>(sx_raw + base);
                uint32_t uy = static_cast<uint32_t>(sy_raw + base);
                uint32_t uz = static_cast<uint32_t>(static_cast<int64_t>(state->timeOffsetRaw) + base);

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
            };

            auto sampleWarp = [&](int64_t sx_raw, int64_t sy_raw, int32_t amp) {
                SPoint32 n = sampleNoisePair(sx_raw, sy_raw);
                int32_t dx = static_cast<int32_t>((static_cast<int64_t>(n.x) * amp) >> 15);
                int32_t dy = static_cast<int32_t>((static_cast<int64_t>(n.y) * amp) >> 15);
                return SPoint32{dx, dy};
            };

            auto sampleCurl = [&](int64_t sx_raw, int64_t sy_raw, int32_t amp) {
                constexpr int32_t EPS = 1 << CARTESIAN_FRAC_BITS;
                uint32_t ux = static_cast<uint32_t>(sx_raw + base);
                uint32_t uy = static_cast<uint32_t>(sy_raw + base);
                uint32_t uz = static_cast<uint32_t>(static_cast<int64_t>(state->timeOffsetRaw) + base);

                auto n_x1 = noiseNormaliseU16(sampleNoiseTrilinear(ux + EPS, uy, uz));
                auto n_x0 = noiseNormaliseU16(sampleNoiseTrilinear(ux - EPS, uy, uz));
                auto n_y1 = noiseNormaliseU16(sampleNoiseTrilinear(ux, uy + EPS, uz));
                auto n_y0 = noiseNormaliseU16(sampleNoiseTrilinear(ux, uy - EPS, uz));

                int32_t dnx = static_cast<int32_t>(raw(n_x1)) - static_cast<int32_t>(raw(n_x0));
                int32_t dny = static_cast<int32_t>(raw(n_y1)) - static_cast<int32_t>(raw(n_y0));

                int32_t dx = static_cast<int32_t>((static_cast<int64_t>(dny) * amp) >> 16);
                int32_t dy = static_cast<int32_t>((static_cast<int64_t>(-dnx) * amp) >> 16);
                return SPoint32{dx, dy};
            };

            SPoint32 warp{0, 0};
            int64_t sx_raw = raw(sx);
            int64_t sy_raw = raw(sy);

            switch (state->type) {
                case WarpType::FBM: {
                    int32_t amp = state->amplitudeRaw;
                    uint8_t octaves = state->octaves ? state->octaves : 1;
                    for (uint8_t o = 0; o < octaves && amp > 0; o++) {
                        int64_t ox = sx_raw << o;
                        int64_t oy = sy_raw << o;
                        SPoint32 step = sampleWarp(ox, oy, amp);
                        warp.x += step.x;
                        warp.y += step.y;
                        amp >>= 1;
                    }
                    break;
                }

                case WarpType::Nested: {
                    SPoint32 first = sampleWarp(sx_raw, sy_raw, state->amplitudeRaw);
                    int64_t wx = sx_raw + first.x;
                    int64_t wy = sy_raw + first.y;
                    int32_t amp = state->amplitudeRaw >> 1;
                    SPoint32 second = sampleWarp(wx << 1, wy << 1, amp);
                    warp.x = first.x + second.x;
                    warp.y = first.y + second.y;
                    break;
                }

                case WarpType::Curl: {
                    warp = sampleCurl(sx_raw, sy_raw, state->amplitudeRaw);
                    break;
                }

                case WarpType::Polar: {
                    int32_t x_q0_16 = static_cast<int32_t>(sx_raw >> CARTESIAN_FRAC_BITS);
                    int32_t y_q0_16 = static_cast<int32_t>(sy_raw >> CARTESIAN_FRAC_BITS);
                    auto [angle, radius] = cartesianToPolar(x_q0_16, y_q0_16);
                    CartQ24_8 ang_q24_8(static_cast<int32_t>(raw(angle)) << CARTESIAN_FRAC_BITS);
                    CartQ24_8 rad_q24_8(static_cast<int32_t>(raw(radius)) << CARTESIAN_FRAC_BITS);
                    ang_q24_8 = CartesianMaths::mul(ang_q24_8, state->warpScale);
                    rad_q24_8 = CartesianMaths::mul(rad_q24_8, state->warpScale);

                    SPoint32 polarNoise = sampleNoisePair(raw(ang_q24_8), raw(rad_q24_8));
                    int64_t polar_amp_64 = static_cast<int64_t>(state->amplitudeRaw) << (16 - CARTESIAN_FRAC_BITS);
                    if (polar_amp_64 > INT32_MAX) polar_amp_64 = INT32_MAX;
                    int32_t polar_amp = static_cast<int32_t>(polar_amp_64);
                    int32_t angle_delta = static_cast<int32_t>((static_cast<int64_t>(polarNoise.x) * polar_amp) >> 16);
                    int32_t radius_delta = static_cast<int32_t>((static_cast<int64_t>(polarNoise.y) * polar_amp) >> 16);

                    uint16_t new_angle = static_cast<uint16_t>(raw(angle) + angle_delta);
                    int32_t new_radius = static_cast<int32_t>(raw(radius)) + radius_delta;
                    if (new_radius < 0) new_radius = 0;
                    if (new_radius > FRACT_Q0_16_MAX) new_radius = FRACT_Q0_16_MAX;

                    auto [px, py] = polarToCartesian(FracQ0_16(new_angle),
                                                     FracQ0_16(static_cast<uint16_t>(new_radius)));
                    int32_t warped_x = px << CARTESIAN_FRAC_BITS;
                    int32_t warped_y = py << CARTESIAN_FRAC_BITS;
                    return layer(CartQ24_8(warped_x), CartQ24_8(warped_y));
                }

                case WarpType::Directional: {
                    warp = sampleWarp(sx_raw, sy_raw, state->amplitudeRaw);
                    warp.x += state->flowOffset.x;
                    warp.y += state->flowOffset.y;
                    break;
                }

                case WarpType::Basic:
                default: {
                    warp = sampleWarp(sx_raw, sy_raw, state->amplitudeRaw);
                    break;
                }
            }

            int32_t warped_x = static_cast<int32_t>(static_cast<int64_t>(raw(x)) + warp.x);
            int32_t warped_y = static_cast<int32_t>(static_cast<int64_t>(raw(y)) + warp.y);
            return layer(CartQ24_8(warped_x), CartQ24_8(warped_y));
        };
    }
}
