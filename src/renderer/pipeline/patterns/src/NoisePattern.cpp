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

#include "renderer/pipeline/patterns/NoisePattern.h"
#include "renderer/pipeline/maths/NoiseMaths.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/signals/ranges/BipolarRange.h"
#if defined(ARDUINO) || defined(__EMSCRIPTEN__)
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif
#include <algorithm>

namespace PolarShader {
    namespace {
        constexpr uint64_t NOISE_DEPTH_FULL_SCALE_WRAP_MS = 6ull * 60ull * 60ull * 1000ull;
        constexpr uint64_t NOISE_DEPTH_SIGNAL_FULL_SCALE = static_cast<uint64_t>(U0X16_MAX);

        const BipolarRange<s0x16> &noiseDepthSpeedSignalRange() {
            static const BipolarRange<s0x16> range{
                s0x16(S0X16_MIN),
                s0x16(S0X16_MAX)
            };
            return range;
        }

        uint32_t random32Seed() {
            return (static_cast<uint32_t>(random16()) << 16) | random16();
        }
    }

    struct NoisePattern::UVNoisePatternFunctor {
        NoiseType type;
        fl::u8 octaves;
        const State *state;

        PatternNormU0x16 operator()(UV uv) const {
            uint32_t offset = NOISE_DOMAIN_OFFSET << 8;
            uint32_t depth = state ? state->depth : 0u;

            // UV is fl::s16x16 (Q16.16) (0.0 .. 1.0, raw 0..65535).
            // We interpret this directly as Cartesian fl::u24x8 (0.0 .. 256.0).
            // This maps the screen width to 256 noise units, restoring the original texture density.
            fl::u24x8 xu = fl::u24x8::from_raw(static_cast<uint32_t>(uv.u.raw()) + offset);
            fl::u24x8 yu = fl::u24x8::from_raw(static_cast<uint32_t>(uv.v.raw()) + offset);
            fl::u24x8 zu = fl::u24x8::from_raw(depth + offset);

            // Two-path cross-dissolve (Basic noise only): evaluate the field at
            // two offset depths and blend so the loop returns seamlessly.
            if (state && state->loopActive && type == NoiseType::Basic) {
                fl::u24x8 zuA = fl::u24x8::from_raw(state->depthA + offset);
                fl::u24x8 zuB = fl::u24x8::from_raw(state->depthB + offset);
                const uint32_t a = raw(noiseLayerImpl(xu, yu, zuA));
                const uint32_t b = raw(noiseLayerImpl(xu, yu, zuB));
                int64_t out = static_cast<int64_t>(a) +
                    (((static_cast<int64_t>(b) - static_cast<int64_t>(a)) *
                      static_cast<int64_t>(state->blendWeight)) >> 16);
                if (out < 0) out = 0;
                if (out > 65535) out = 65535;
                return PatternNormU0x16::from_raw(static_cast<uint16_t>(out));
            }

            switch (type) {
                case NoiseType::FBM:
                    return fBmLayerImpl(xu, yu, zu, octaves);
                case NoiseType::Turbulence:
                    return turbulenceLayerImpl(xu, yu, zu);
                case NoiseType::Ridged:
                    return ridgedLayerImpl(xu, yu, zu);
                case NoiseType::Basic:
                default:
                    return noiseLayerImpl(xu, yu, zu);
            }
        }
    };

    PatternNormU0x16 NoisePattern::noiseLayerImpl(fl::u24x8 x, fl::u24x8 y, fl::u24x8 z) {
        return noiseNormaliseU16(sampleNoiseTrilinear(x.raw(), y.raw(), z.raw()));
    }

    PatternNormU0x16 NoisePattern::fBmLayerImpl(fl::u24x8 x, fl::u24x8 y, fl::u24x8 z, fl::u8 octaveCount) {
        uint32_t r = 0;
        uint16_t amplitude = U16_HALF;
        for (int o = 0; o < octaveCount; o++) {
            auto n = sampleNoiseTrilinear(x.raw(), y.raw(), z.raw());
            r += (static_cast<uint32_t>(raw(n)) * amplitude) >> 16;
            x = fl::u24x8::from_raw(x.raw() << 1);
            y = fl::u24x8::from_raw(y.raw() << 1);
            z = fl::u24x8::from_raw(z.raw() << 1);
            amplitude >>= 1;
        }
        if (r > UINT16_MAX) r = UINT16_MAX;
        return noiseNormaliseU16(NoiseRawU0x16(static_cast<uint16_t>(r)));
    }

    PatternNormU0x16 NoisePattern::turbulenceLayerImpl(fl::u24x8 x, fl::u24x8 y, fl::u24x8 z) {
        NoiseRawU0x16 noise_raw = NoiseRawU0x16(sampleNoiseTrilinear(x.raw(), y.raw(), z.raw()));
        int16_t r = static_cast<int16_t>(raw(noise_raw)) - U16_HALF;
        uint16_t mag = static_cast<uint16_t>(r ^ (r >> 15)) - static_cast<uint16_t>(r >> 15);
        uint32_t doubled = static_cast<uint32_t>(mag) << 1;
        if (doubled > S0X16_MAX) doubled = S0X16_MAX;
        return noiseNormaliseU16(NoiseRawU0x16(static_cast<uint16_t>(doubled)));
    }

    PatternNormU0x16 NoisePattern::ridgedLayerImpl(fl::u24x8 x, fl::u24x8 y, fl::u24x8 z) {
        NoiseRawU0x16 noise_raw = NoiseRawU0x16(sampleNoiseTrilinear(x.raw(), y.raw(), z.raw()));
        int16_t r = static_cast<int16_t>(raw(noise_raw)) - U16_HALF;
        uint16_t mag = static_cast<uint16_t>(r ^ (r >> 15)) - static_cast<uint16_t>(r >> 15);
        mag = std::min(mag, static_cast<uint16_t>(U16_HALF - 1));
        uint32_t doubled = static_cast<uint32_t>(mag) << 1;
        if (doubled > S0X16_MAX) doubled = S0X16_MAX;
        uint16_t inverted = static_cast<uint16_t>(S0X16_MAX - doubled);
        return noiseNormaliseU16(NoiseRawU0x16(inverted));
    }

    NoisePattern::NoisePattern(
        NoiseType noiseType,
        fl::u8 octaveCount,
        S0x16Signal depthSpeedSignal,
        bool loopEnabled,
        uint16_t loopPeriodMs
    )
        : type(noiseType),
          octaves(octaveCount),
          depthSpeedSignal(depthSpeedSignal ? std::move(depthSpeedSignal) : cRandom()),
          loopEnabled(loopEnabled),
          loopPeriodMs(loopPeriodMs) {
        state.depth = random32Seed();
    }

    void NoisePattern::advanceFrame(u0x16 progress, TimeMillis elapsedMs) {
        (void) progress;

        // Two-path cross-dissolve loop. Runs BEFORE the first-frame early return
        // below because it is a pure function of the absolute elapsed time (no
        // delta priming needed): out(phi) = lerp(noise(zA), noise(zB), w(phi))
        // with zA = base + phi*L, zB = zA - L, and a monotonic 0->1 weight.
        if (loopEnabled && loopPeriodMs > 0) {
            if (!state.loopInitialized) {
                // Sample the speed signal ONCE at the fixed epoch elapsedMs = 0,
                // so a cold first call at any time chooses the same span as a
                // warmed scene (the loop stays a pure function of absolute time).
                const s0x16 signedSpeed =
                    depthSpeedSignal ? depthSpeedSignal.sample(noiseDepthSpeedSignalRange(), 0u)
                                     : s0x16(0);
                const uint64_t speed = static_cast<uint64_t>(raw(toUnsignedClamped(signedSpeed)));
                const uint64_t denominator = NOISE_DEPTH_SIGNAL_FULL_SCALE * NOISE_DEPTH_FULL_SCALE_WRAP_MS;
                const uint64_t span =
                    speed * static_cast<uint64_t>(UINT32_MAX) * static_cast<uint64_t>(loopPeriodMs) / denominator;
                state.loopSpan = static_cast<uint32_t>(span);
                state.loopBaseDepth = state.depth;
                state.loopInitialized = true;
            }

            const uint16_t phi_raw = static_cast<uint16_t>(
                static_cast<uint64_t>(elapsedMs % loopPeriodMs) * 65535u / loopPeriodMs);
            state.depthA = state.loopBaseDepth +
                static_cast<uint32_t>((static_cast<uint64_t>(phi_raw) * state.loopSpan) >> 16);
            state.depthB = state.depthA - state.loopSpan;

            int32_t w = 32767 - static_cast<int32_t>(cos16(static_cast<uint16_t>(phi_raw >> 1)));
            if (w < 0) w = 0;
            if (w > 65535) w = 65535;
            state.blendWeight = static_cast<uint16_t>(w);
            state.loopActive = true;
            return;
        }

        if (!state.hasLastElapsed) {
            state.lastElapsedMs = elapsedMs;
            state.hasLastElapsed = true;
            return;
        }

        int64_t deltaMs = static_cast<int64_t>(elapsedMs) - static_cast<int64_t>(state.lastElapsedMs);
        state.lastElapsedMs = elapsedMs;

        if (MAX_DELTA_TIME_MS != 0) {
            const int64_t maxDelta = MAX_DELTA_TIME_MS;
            if (deltaMs > maxDelta) deltaMs = maxDelta;
            if (deltaMs < -maxDelta) deltaMs = -maxDelta;
        }

        if (deltaMs <= 0 || !depthSpeedSignal) return;

        const s0x16 signedSpeed = depthSpeedSignal.sample(noiseDepthSpeedSignalRange(), elapsedMs);
        const uint64_t speed = static_cast<uint64_t>(raw(toUnsignedClamped(signedSpeed)));
        const uint64_t denominator = NOISE_DEPTH_SIGNAL_FULL_SCALE * NOISE_DEPTH_FULL_SCALE_WRAP_MS;
        const uint64_t numerator =
            speed * static_cast<uint64_t>(UINT32_MAX) * static_cast<uint64_t>(deltaMs) + state.depthRemainder;
        const uint64_t increment = numerator / denominator;
        state.depthRemainder = numerator % denominator;
        state.depth += static_cast<uint32_t>(increment);
    }

    UVMap NoisePattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        (void) context;
        return UVNoisePatternFunctor{type, octaves, &state};
    }
}
