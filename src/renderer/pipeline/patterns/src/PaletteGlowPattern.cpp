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

#include "renderer/pipeline/patterns/PaletteGlowPattern.h"
#include "renderer/pipeline/maths/ShaderMaths.h"
#include <limits.h>

namespace PolarShader {
    namespace {
        constexpr int32_t Q16_HALF = 0x00008000;
        constexpr int32_t Q16_TILE_SCALE = 0x00018000; // 1.5
        constexpr uint32_t Q16_TIME_SCALE = 26214u; // 0.4
        constexpr uint32_t Q16_GLOW_BASE = 655u; // 0.01
        constexpr uint32_t Q16_GLOW_DIV_MAX = 16u << 16;
        constexpr uint32_t Q16_WAVE_EPSILON = 8u;
        constexpr uint32_t Q16_INV_TAU = 10430u;

        int32_t displayAspectRaw(const std::shared_ptr<PipelineContext> &context) {
            if (!(context && context->rasterDisplay.valid && context->rasterDisplay.height > 0)) {
                return SF16_ONE;
            }
            uint64_t scaled = static_cast<uint64_t>(context->rasterDisplay.width) << 16;
            return static_cast<int32_t>(scaled / context->rasterDisplay.height);
        }

        fl::s16x16 shaderAxis(fl::s16x16 axis) {
            int64_t centred = static_cast<int64_t>(raw(axis)) * 2 - SF16_ONE;
            if (centred > INT32_MAX) centred = INT32_MAX;
            if (centred < INT32_MIN) centred = INT32_MIN;
            return fl::s16x16::from_raw(static_cast<int32_t>(centred));
        }

        Vec2Q16 shaderUv(UV uv, int32_t aspectRaw) {
            return Vec2Q16{
                fl::s16x16::from_raw(mulQ16Raw(raw(shaderAxis(uv.u)), aspectRaw)),
                shaderAxis(uv.v)
            };
        }

        fl::s16x16 tileAxis(fl::s16x16 axis) {
            int32_t scaled = mulQ16Raw(raw(axis), Q16_TILE_SCALE);
            return fl::s16x16::from_raw(raw(fractQ16(fl::s16x16::from_raw(scaled))) - Q16_HALF);
        }

        Vec2Q16 tileUv(Vec2Q16 uv) {
            return Vec2Q16{tileAxis(uv.x), tileAxis(uv.y)};
        }

        uint32_t scaleByGlow(uint16_t channel, uint32_t glowRaw) {
            uint64_t value = (static_cast<uint64_t>(channel) * glowRaw) >> 16;
            return value > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(value);
        }

        void saturatingAdd(uint32_t &target, uint32_t value) {
            uint32_t previous = target;
            target += value;
            if (target < previous) target = UINT32_MAX;
        }

        uint16_t clampAccumToU16(uint32_t value) {
            return value > F16_MAX ? F16_MAX : static_cast<uint16_t>(value);
        }

        PatternNormU16 unpremultiplyChannel(uint32_t channel, uint32_t maxChannel) {
            if (maxChannel == 0u) return PatternNormU16(0);
            if (maxChannel > F16_MAX) return PatternNormU16(clampAccumToU16(channel));

            uint64_t scaled = (static_cast<uint64_t>(channel) << 16) / maxChannel;
            if (scaled > F16_MAX) scaled = F16_MAX;
            return PatternNormU16(static_cast<uint16_t>(scaled));
        }

        uint32_t shaderWaveGlow(uint32_t distanceRaw, uint32_t timeRaw) {
            uint32_t waveRadiansRaw = (distanceRaw << 3) + timeRaw;
            uint32_t waveTurnsRaw = static_cast<uint32_t>(
                (static_cast<uint64_t>(waveRadiansRaw) * Q16_INV_TAU) >> 16
            );
            int32_t wave = raw(angleSinF16(f16(static_cast<uint16_t>(waveTurnsRaw))));
            if (wave < 0) wave = -wave;
            uint32_t waveRaw = static_cast<uint32_t>(wave) >> 3;
            if (waveRaw < Q16_WAVE_EPSILON) waveRaw = Q16_WAVE_EPSILON;

            fl::u16x16 inverse = divClampedQ16(
                fl::u16x16::from_raw(Q16_GLOW_BASE),
                fl::u16x16::from_raw(waveRaw),
                fl::u16x16::from_raw(Q16_GLOW_DIV_MAX)
            );
            return raw(powQ16(inverse, 1200));
        }
    }

    struct PaletteGlowPattern::State {
        uint32_t timeRaw{0};
    };

    struct PaletteGlowPattern::Functor {
        const State *state;
        int32_t aspectRaw;

        RgbSample sample(UV uv) const {
            Vec2Q16 current = shaderUv(uv, aspectRaw);
            const Vec2Q16 uv0 = current;
            const uint32_t uv0LengthRaw = static_cast<uint32_t>(raw(lengthQ16(uv0)));
            const fl::u16x16 falloff = expNegQ16(fl::u16x16::from_raw(uv0LengthRaw));
            const uint32_t timeRaw = state ? state->timeRaw : 0u;

            uint32_t accR = 0;
            uint32_t accG = 0;
            uint32_t accB = 0;

            for (uint8_t i = 0; i < 4; ++i) {
                current = tileUv(current);

                uint32_t dRaw = static_cast<uint32_t>(
                    (static_cast<uint64_t>(static_cast<uint32_t>(raw(lengthQ16(current)))) * raw(falloff)) >> 16
                );

                uint32_t paletteTRaw =
                    uv0LengthRaw +
                    (static_cast<uint32_t>(i) * Q16_TIME_SCALE) +
                    static_cast<uint32_t>((static_cast<uint64_t>(timeRaw) * Q16_TIME_SCALE) >> 16);
                RgbSample colour = iqCosinePaletteQ16(fl::u16x16::from_raw(paletteTRaw), PatternNormU16(F16_MAX));

                uint32_t glowRaw = shaderWaveGlow(dRaw, timeRaw);

                saturatingAdd(accR, scaleByGlow(raw(colour.red()), glowRaw));
                saturatingAdd(accG, scaleByGlow(raw(colour.green()), glowRaw));
                saturatingAdd(accB, scaleByGlow(raw(colour.blue()), glowRaw));
            }

            uint32_t maxChannel = accR;
            if (accG > maxChannel) maxChannel = accG;
            if (accB > maxChannel) maxChannel = accB;
            if (maxChannel == 0u) return RgbSample();

            PatternNormU16 value(clampAccumToU16(maxChannel));
            return RgbSample(
                unpremultiplyChannel(accR, maxChannel),
                unpremultiplyChannel(accG, maxChannel),
                unpremultiplyChannel(accB, maxChannel),
                value
            );
        }

        PatternNormU16 operator()(UV uv) const {
            return sample(uv).value();
        }
    };

    PaletteGlowPattern::PaletteGlowPattern()
        : state(std::make_shared<State>()) {}

    void PaletteGlowPattern::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void)progress;
        state->timeRaw = static_cast<uint32_t>((static_cast<uint64_t>(elapsedMs) << 16) / 1000u);
    }

    UVMap PaletteGlowPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        return Functor{state.get(), displayAspectRaw(context)};
    }

    UVLayer PaletteGlowPattern::uvLayer(const std::shared_ptr<PipelineContext> &context) const {
        return UVLayer::fromRgb([f = Functor{state.get(), displayAspectRaw(context)}](UV uv) {
            return f.sample(uv);
        });
    }
}
