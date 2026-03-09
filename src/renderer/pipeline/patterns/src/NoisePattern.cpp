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
#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif
#include <algorithm>

namespace PolarShader {
    namespace {
        constexpr uint64_t NOISE_DEPTH_FULL_SCALE_WRAP_MS = 6ull * 60ull * 60ull * 1000ull;
        constexpr uint64_t NOISE_DEPTH_SIGNAL_FULL_SCALE = static_cast<uint64_t>(F16_MAX);

        const BipolarRange<sf16> &noiseDepthSpeedSignalRange() {
            static const BipolarRange<sf16> range{
                sf16(SF16_MIN),
                sf16(SF16_MAX)
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

        PatternNormU16 operator()(UV uv) const {
            uint32_t offset = NOISE_DOMAIN_OFFSET << R8_FRAC_BITS;
            uint32_t depth = state ? state->depth : 0u;

            // UV is r16/sr16 (Q16.16) (0.0 .. 1.0, raw 0..65535).
            // We interpret this directly as Cartesian sr8/r8 (0.0 .. 256.0).
            // This maps the screen width to 256 noise units, restoring the original texture density.
            r8 xu(static_cast<uint32_t>(raw(uv.u)) + offset);
            r8 yu(static_cast<uint32_t>(raw(uv.v)) + offset);
            r8 zu(depth + offset);

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

    PatternNormU16 NoisePattern::noiseLayerImpl(r8 x, r8 y, r8 z) {
        return noiseNormaliseU16(sampleNoiseTrilinear(raw(x), raw(y), raw(z)));
    }

    PatternNormU16 NoisePattern::fBmLayerImpl(r8 x, r8 y, r8 z, fl::u8 octaveCount) {
        uint32_t r = 0;
        uint16_t amplitude = U16_HALF;
        for (int o = 0; o < octaveCount; o++) {
            auto n = sampleNoiseTrilinear(raw(x), raw(y), raw(z));
            r += (static_cast<uint32_t>(raw(n)) * amplitude) >> 16;
            x = r8(raw(x) << 1);
            y = r8(raw(y) << 1);
            z = r8(raw(z) << 1);
            amplitude >>= 1;
        }
        if (r > UINT16_MAX) r = UINT16_MAX;
        return noiseNormaliseU16(NoiseRawU16(static_cast<uint16_t>(r)));
    }

    PatternNormU16 NoisePattern::turbulenceLayerImpl(r8 x, r8 y, r8 z) {
        NoiseRawU16 noise_raw = NoiseRawU16(sampleNoiseTrilinear(raw(x), raw(y), raw(z)));
        int16_t r = static_cast<int16_t>(raw(noise_raw)) - U16_HALF;
        uint16_t mag = static_cast<uint16_t>(r ^ (r >> 15)) - static_cast<uint16_t>(r >> 15);
        uint32_t doubled = static_cast<uint32_t>(mag) << 1;
        if (doubled > SF16_MAX) doubled = SF16_MAX;
        return noiseNormaliseU16(NoiseRawU16(static_cast<uint16_t>(doubled)));
    }

    PatternNormU16 NoisePattern::ridgedLayerImpl(r8 x, r8 y, r8 z) {
        NoiseRawU16 noise_raw = NoiseRawU16(sampleNoiseTrilinear(raw(x), raw(y), raw(z)));
        int16_t r = static_cast<int16_t>(raw(noise_raw)) - U16_HALF;
        uint16_t mag = static_cast<uint16_t>(r ^ (r >> 15)) - static_cast<uint16_t>(r >> 15);
        mag = std::min(mag, static_cast<uint16_t>(U16_HALF - 1));
        uint32_t doubled = static_cast<uint32_t>(mag) << 1;
        if (doubled > SF16_MAX) doubled = SF16_MAX;
        uint16_t inverted = static_cast<uint16_t>(SF16_MAX - doubled);
        return noiseNormaliseU16(NoiseRawU16(inverted));
    }

    NoisePattern::NoisePattern(
        NoiseType noiseType,
        fl::u8 octaveCount,
        Sf16Signal depthSpeedSignal
    )
        : type(noiseType),
          octaves(octaveCount),
          depthSpeedSignal(depthSpeedSignal ? std::move(depthSpeedSignal) : cRandom()) {
        state.depth = random32Seed();
    }

    void NoisePattern::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void) progress;
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

        const sf16 signedSpeed = depthSpeedSignal.sample(noiseDepthSpeedSignalRange(), elapsedMs);
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
