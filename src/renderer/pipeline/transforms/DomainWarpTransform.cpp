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
#include "renderer/pipeline/ranges/LinearRange.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/signals/Accumulators.h"
#include "renderer/pipeline/signals/Signals.h"
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
        MappedSignal<SFracQ0_16> phaseSpeedSignal;
        MappedSignal<int32_t> amplitudeSignal;
        MappedSignal<CartQ24_8> warpScaleSignal;
        std::shared_ptr<MappedSignal<CartQ24_8>> maxOffsetSignal;
        MappedSignal<FracQ0_16> flowDirectionSignal;
        MappedSignal<int32_t> flowStrengthSignal;
    };

    DomainWarpTransform::MappedInputs DomainWarpTransform::makeInputs(
        SFracQ0_16Signal speed,
        SFracQ0_16Signal amplitude,
        SFracQ0_16Signal warpScale,
        SFracQ0_16Signal maxOffset,
        LinearRange<CartQ24_8> warpScaleRange,
        LinearRange<CartQ24_8> maxOffsetRange,
        SFracQ0_16Signal flowDirection,
        SFracQ0_16Signal flowStrength
    ) {
        bool hasFlow = static_cast<bool>(flowDirection) && static_cast<bool>(flowStrength);

        auto maxOffsetSignal = std::make_shared<MappedSignal<CartQ24_8>>(
            resolveMappedSignal(maxOffsetRange.mapSignal(std::move(maxOffset)))
        );

        auto mapToMaxOffset = [maxOffsetSignal](SFracQ0_16Signal signal) -> MappedSignal<int32_t> {
            bool absolute = signal.isAbsolute();
            return MappedSignal<int32_t>(
                [signal = std::move(signal), maxOffsetSignal](FracQ0_16 progress, TimeMillis elapsedMs) mutable -> MappedValue<int32_t> {
                    int32_t max_raw = raw((*maxOffsetSignal)(progress, elapsedMs).get());
                    if (max_raw <= 0) return MappedValue<int32_t>(0);
                    uint32_t t_raw = clamp_frac_raw(raw(signal(progress, elapsedMs)));
                    int64_t scaled = (static_cast<int64_t>(max_raw) * static_cast<int64_t>(t_raw)) >> 16;
                    return MappedValue(static_cast<int32_t>(scaled));
                },
                absolute
            );
        };

        PolarRange flowDirectionRange;
        
        // Use speed signal directly for MappedInputs.
        auto speedMapped = MappedSignal<SFracQ0_16>(
            [speed](FracQ0_16 p, TimeMillis t) {
                return MappedValue(speed(p, t));
            },
            speed.isAbsolute()
        );

        return DomainWarpTransform::MappedInputs{
            resolveMappedSignal(std::move(speedMapped)),
            resolveMappedSignal(mapToMaxOffset(std::move(amplitude))),
            resolveMappedSignal(warpScaleRange.mapSignal(std::move(warpScale))),
            std::move(maxOffsetSignal),
            hasFlow
                ? resolveMappedSignal(flowDirectionRange.mapSignal(std::move(flowDirection)))
                : MappedSignal<FracQ0_16>(),
            hasFlow ? resolveMappedSignal(mapToMaxOffset(std::move(flowStrength))) : MappedSignal<int32_t>()
        };
    }

    struct DomainWarpTransform::State {
        WarpType type;
        PhaseAccumulator phase;
        MappedSignal<int32_t> amplitudeSignal;
        MappedSignal<CartQ24_8> warpScaleSignal;
        std::shared_ptr<MappedSignal<CartQ24_8>> maxOffsetSignal;
        CartQ24_8 warpScale = CartQ24_8(0);
        CartQ24_8 maxOffset = CartQ24_8(0);
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
            MappedSignal<CartQ24_8> scale,
            std::shared_ptr<MappedSignal<CartQ24_8>> offset,
            uint8_t octaves,
            MappedSignal<FracQ0_16> flowDirection,
            MappedSignal<int32_t> flowStrength
        ) : type(type),
            phase(std::move(phaseSpeed)),
            amplitudeSignal(std::move(amplitude)),
            warpScaleSignal(std::move(scale)),
            maxOffsetSignal(std::move(offset)),
            octaves(octaves),
            flowDirectionSignal(std::move(flowDirection)),
            flowStrengthSignal(std::move(flowStrength)) {
        }
    };

    DomainWarpTransform::DomainWarpTransform(
        SFracQ0_16Signal speed,
        SFracQ0_16Signal amplitude,
        SFracQ0_16Signal warpScale,
        SFracQ0_16Signal maxOffset,
        LinearRange<CartQ24_8> warpScaleRange,
        LinearRange<CartQ24_8> maxOffsetRange
    ) : DomainWarpTransform(WarpType::Basic, std::move(speed), amplitude, warpScale, maxOffset, warpScaleRange, maxOffsetRange, 3) {
    }

    DomainWarpTransform::DomainWarpTransform(
        WarpType type,
        SFracQ0_16Signal speed,
        SFracQ0_16Signal amplitude,
        SFracQ0_16Signal warpScale,
        SFracQ0_16Signal maxOffset,
        LinearRange<CartQ24_8> warpScaleRange,
        LinearRange<CartQ24_8> maxOffsetRange,
        uint8_t octaves,
        SFracQ0_16Signal flowDirection,
        SFracQ0_16Signal flowStrength
    ) {
        auto inputs = makeInputs(std::move(speed), amplitude, warpScale, maxOffset, warpScaleRange, maxOffsetRange, flowDirection, flowStrength);
        state = std::make_shared<State>(
            type,
            std::move(inputs.phaseSpeedSignal),
            std::move(inputs.amplitudeSignal),
            std::move(inputs.warpScaleSignal),
            std::move(inputs.maxOffsetSignal),
            octaves,
            std::move(inputs.flowDirectionSignal),
            std::move(inputs.flowStrengthSignal)
        );
    }

    void DomainWarpTransform::advanceFrame(FracQ0_16 progress, TimeMillis elapsedMs) {
        if (!context) {
            // Serial.println("DomainWarpTransform::advanceFrame context is null.");
        }

        FracQ0_16 phase = state->phase.advance(progress, elapsedMs);
        state->timeOffsetRaw = static_cast<int32_t>(raw(phase)) << CARTESIAN_FRAC_BITS;

        state->warpScale = state->warpScaleSignal(progress, elapsedMs).get();
        state->maxOffset = (*state->maxOffsetSignal)(progress, elapsedMs).get();

        state->amplitudeRaw = state->amplitudeSignal(progress, elapsedMs).get();

        if (state->type == WarpType::Directional && state->flowDirectionSignal && state->flowStrengthSignal) {
            FracQ0_16 dir = state->flowDirectionSignal(progress, elapsedMs).get();
            int32_t strength = state->flowStrengthSignal(progress, elapsedMs).get();
            SFracQ0_16 cos_q0_16 = angleCosQ0_16(dir);
            SFracQ0_16 sin_q0_16 = angleSinQ0_16(dir);
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
            CartQ24_8 cx = CartesianMaths::from_uv(uv.u);
            CartQ24_8 cy = CartesianMaths::from_uv(uv.v);

            CartQ24_8 sx = CartesianMaths::mul(cx, state->warpScale);
            CartQ24_8 sy = CartesianMaths::mul(cy, state->warpScale);

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

                    polar_uv.u = FracQ16_16(raw(polar_uv.u) + angle_delta);
                    polar_uv.v = FracQ16_16(raw(polar_uv.v) + radius_delta);

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
                FracQ16_16(raw(uv.u) + (warp.x << 8)),
                FracQ16_16(raw(uv.v) + (warp.y << 8))
            );
            return layer(warped_uv);
        };
    }
}