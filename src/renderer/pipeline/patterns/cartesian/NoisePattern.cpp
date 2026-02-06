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
#include <Arduino.h>
#include <algorithm>

namespace PolarShader {
    struct NoisePattern::UVNoisePatternFunctor {
        NoiseType type;
        fl::u8 octaves;
        PipelineContext *context;

        PatternNormU16 operator()(UV uv) const {
            int64_t offset = static_cast<int64_t>(NOISE_DOMAIN_OFFSET) << CARTESIAN_FRAC_BITS;
            uint32_t depth = context ? context->depth : 0u;

            // UV is Q16.16, we want CartUQ24.8. 
            // Shift right by 8 to get 8 fractional bits.
            uint32_t ux = (static_cast<uint32_t>(raw(uv.u)) >> 8) + static_cast<uint32_t>(offset);
            uint32_t uy = (static_cast<uint32_t>(raw(uv.v)) >> 8) + static_cast<uint32_t>(offset);
            uint32_t uz = depth + static_cast<uint32_t>(offset);

            CartUQ24_8 xu(ux);
            CartUQ24_8 yu(uy);
            CartUQ24_8 zu(uz);

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

    PatternNormU16 NoisePattern::noiseLayerImpl(CartUQ24_8 x, CartUQ24_8 y, CartUQ24_8 z) {
        return noiseNormaliseU16(sampleNoiseTrilinear(raw(x), raw(y), raw(z)));
    }

    PatternNormU16 NoisePattern::fBmLayerImpl(CartUQ24_8 x, CartUQ24_8 y, CartUQ24_8 z, fl::u8 octaveCount) {
        uint32_t r = 0;
        uint16_t amplitude = U16_HALF;
        for (int o = 0; o < octaveCount; o++) {
            auto n = sampleNoiseTrilinear(raw(x), raw(y), raw(z));
            r += (static_cast<uint32_t>(raw(n)) * amplitude) >> 16;
            x = CartUQ24_8(raw(x) << 1);
            y = CartUQ24_8(raw(y) << 1);
            z = CartUQ24_8(raw(z) << 1);
            amplitude >>= 1;
        }
        if (r > UINT16_MAX) r = UINT16_MAX;
        return noiseNormaliseU16(NoiseRawU16(static_cast<uint16_t>(r)));
    }

    PatternNormU16 NoisePattern::turbulenceLayerImpl(CartUQ24_8 x, CartUQ24_8 y, CartUQ24_8 z) {
        NoiseRawU16 noise_raw = NoiseRawU16(sampleNoiseTrilinear(raw(x), raw(y), raw(z)));
        int16_t r = static_cast<int16_t>(raw(noise_raw)) - U16_HALF;
        uint16_t mag = static_cast<uint16_t>(r ^ (r >> 15)) - static_cast<uint16_t>(r >> 15);
        uint32_t doubled = static_cast<uint32_t>(mag) << 1;
        if (doubled > FRACT_Q0_16_MAX) doubled = FRACT_Q0_16_MAX;
        return noiseNormaliseU16(NoiseRawU16(static_cast<uint16_t>(doubled)));
    }

    PatternNormU16 NoisePattern::ridgedLayerImpl(CartUQ24_8 x, CartUQ24_8 y, CartUQ24_8 z) {
        NoiseRawU16 noise_raw = NoiseRawU16(sampleNoiseTrilinear(raw(x), raw(y), raw(z)));
        int16_t r = static_cast<int16_t>(raw(noise_raw)) - U16_HALF;
        uint16_t mag = static_cast<uint16_t>(r ^ (r >> 15)) - static_cast<uint16_t>(r >> 15);
        mag = std::min(mag, static_cast<uint16_t>(U16_HALF - 1));
        uint32_t doubled = static_cast<uint32_t>(mag) << 1;
        if (doubled > FRACT_Q0_16_MAX) doubled = FRACT_Q0_16_MAX;
        uint16_t inverted = static_cast<uint16_t>(FRACT_Q0_16_MAX - doubled);
        return noiseNormaliseU16(NoiseRawU16(inverted));
    }

    NoisePattern::NoisePattern(NoiseType noiseType, fl::u8 octaveCount)
        : type(noiseType),
          octaves(octaveCount) {
    }

    UVLayer NoisePattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        return UVNoisePatternFunctor{type, octaves, context.get()};
    }
}
