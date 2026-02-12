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

#include "NoisePattern.h"
#include "renderer/pipeline/maths/NoiseMaths.h"
#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif
#include <algorithm>

namespace PolarShader {
    struct NoisePattern::UVNoisePatternFunctor {
        NoiseType type;
        fl::u8 octaves;
        PipelineContext *context;

        PatternNormU16 operator()(UV uv) const {
            uint32_t offset = NOISE_DOMAIN_OFFSET << CARTESIAN_FRAC_BITS;
            uint32_t depth = context ? context->depth : 0u;

            // UV is Q16.16 (0.0 .. 1.0, raw 0..65535).
            // We interpret this directly as Cartesian Q24.8 (0.0 .. 256.0).
            // This maps the screen width to 256 noise units, restoring the original texture density.
            UQ24_8 xu(static_cast<uint32_t>(raw(uv.u)) + offset);
            UQ24_8 yu(static_cast<uint32_t>(raw(uv.v)) + offset);
            UQ24_8 zu(depth + offset);

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

    PatternNormU16 NoisePattern::noiseLayerImpl(UQ24_8 x, UQ24_8 y, UQ24_8 z) {
        return noiseNormaliseU16(sampleNoiseTrilinear(raw(x), raw(y), raw(z)));
    }

    PatternNormU16 NoisePattern::fBmLayerImpl(UQ24_8 x, UQ24_8 y, UQ24_8 z, fl::u8 octaveCount) {
        uint32_t r = 0;
        uint16_t amplitude = U16_HALF;
        for (int o = 0; o < octaveCount; o++) {
            auto n = sampleNoiseTrilinear(raw(x), raw(y), raw(z));
            r += (static_cast<uint32_t>(raw(n)) * amplitude) >> 16;
            x = UQ24_8(raw(x) << 1);
            y = UQ24_8(raw(y) << 1);
            z = UQ24_8(raw(z) << 1);
            amplitude >>= 1;
        }
        if (r > UINT16_MAX) r = UINT16_MAX;
        return noiseNormaliseU16(NoiseRawU16(static_cast<uint16_t>(r)));
    }

    PatternNormU16 NoisePattern::turbulenceLayerImpl(UQ24_8 x, UQ24_8 y, UQ24_8 z) {
        NoiseRawU16 noise_raw = NoiseRawU16(sampleNoiseTrilinear(raw(x), raw(y), raw(z)));
        int16_t r = static_cast<int16_t>(raw(noise_raw)) - U16_HALF;
        uint16_t mag = static_cast<uint16_t>(r ^ (r >> 15)) - static_cast<uint16_t>(r >> 15);
        uint32_t doubled = static_cast<uint32_t>(mag) << 1;
        if (doubled > SQ0_16_MAX) doubled = SQ0_16_MAX;
        return noiseNormaliseU16(NoiseRawU16(static_cast<uint16_t>(doubled)));
    }

    PatternNormU16 NoisePattern::ridgedLayerImpl(UQ24_8 x, UQ24_8 y, UQ24_8 z) {
        NoiseRawU16 noise_raw = NoiseRawU16(sampleNoiseTrilinear(raw(x), raw(y), raw(z)));
        int16_t r = static_cast<int16_t>(raw(noise_raw)) - U16_HALF;
        uint16_t mag = static_cast<uint16_t>(r ^ (r >> 15)) - static_cast<uint16_t>(r >> 15);
        mag = std::min(mag, static_cast<uint16_t>(U16_HALF - 1));
        uint32_t doubled = static_cast<uint32_t>(mag) << 1;
        if (doubled > SQ0_16_MAX) doubled = SQ0_16_MAX;
        uint16_t inverted = static_cast<uint16_t>(SQ0_16_MAX - doubled);
        return noiseNormaliseU16(NoiseRawU16(inverted));
    }

    NoisePattern::NoisePattern(NoiseType noiseType, fl::u8 octaveCount)
        : type(noiseType),
          octaves(octaveCount) {
    }

    UVMap NoisePattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        return UVNoisePatternFunctor{type, octaves, context.get()};
    }
}
