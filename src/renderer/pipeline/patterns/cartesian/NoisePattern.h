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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_CARTESIAN_CARTESIANNOISEPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_CARTESIAN_CARTESIANNOISEPATTERN_H

#include "FastLED.h"
#include "renderer/pipeline/maths/NoiseMaths.h"
#include "renderer/pipeline/patterns/BasePattern.h"

namespace PolarShader {
    class NoisePattern : public CartesianPattern {
    public:
        enum class NoiseType {
            Basic,
            FBM,
            Turbulence,
            Ridged
        };

    private:
        struct NoisePatternFunctor {
            NoiseType type;
            fl::u8 octaves;

            PatternNormU16 operator()(CartQ24_8 x, CartQ24_8 y, uint32_t depth) const {
                int64_t offset = static_cast<int64_t>(NOISE_DOMAIN_OFFSET) << CARTESIAN_FRAC_BITS;
                int64_t sx = static_cast<int64_t>(raw(x)) + offset;
                int64_t sy = static_cast<int64_t>(raw(y)) + offset;
                int64_t sz = static_cast<int64_t>(depth) + offset;
                uint32_t ux = static_cast<uint32_t>(sx);
                uint32_t uy = static_cast<uint32_t>(sy);
                uint32_t uz = static_cast<uint32_t>(sz);
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

        NoiseType type;
        fl::u8 octaves;

        static PatternNormU16 noiseLayerImpl(CartUQ24_8 x, CartUQ24_8 y, CartUQ24_8 z) {
            return noiseNormaliseU16(sampleNoiseTrilinear(raw(x), raw(y), raw(z)));
        }

        static PatternNormU16 fBmLayerImpl(CartUQ24_8 x, CartUQ24_8 y, CartUQ24_8 z, fl::u8 octaveCount) {
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

        static PatternNormU16 turbulenceLayerImpl(CartUQ24_8 x, CartUQ24_8 y, CartUQ24_8 z) {
            NoiseRawU16 noise_raw = NoiseRawU16(sampleNoiseTrilinear(raw(x), raw(y), raw(z)));
            int16_t r = static_cast<int16_t>(raw(noise_raw)) - U16_HALF;
            uint16_t mag = static_cast<uint16_t>(r ^ (r >> 15)) - static_cast<uint16_t>(r >> 15);
            uint32_t doubled = static_cast<uint32_t>(mag) << 1;
            if (doubled > FRACT_Q0_16_MAX) doubled = FRACT_Q0_16_MAX;
            return noiseNormaliseU16(NoiseRawU16(static_cast<uint16_t>(doubled)));
        }

        static PatternNormU16 ridgedLayerImpl(CartUQ24_8 x, CartUQ24_8 y, CartUQ24_8 z) {
            NoiseRawU16 noise_raw = NoiseRawU16(sampleNoiseTrilinear(raw(x), raw(y), raw(z)));
            int16_t r = static_cast<int16_t>(raw(noise_raw)) - U16_HALF;
            uint16_t mag = static_cast<uint16_t>(r ^ (r >> 15)) - static_cast<uint16_t>(r >> 15);
            mag = min<uint16_t>(mag, static_cast<uint16_t>(U16_HALF - 1));
            uint32_t doubled = static_cast<uint32_t>(mag) << 1;
            if (doubled > FRACT_Q0_16_MAX) doubled = FRACT_Q0_16_MAX;
            uint16_t inverted = static_cast<uint16_t>(FRACT_Q0_16_MAX - doubled);
            return noiseNormaliseU16(NoiseRawU16(inverted));
        }

    public:
        explicit NoisePattern(NoiseType noiseType = NoiseType::Basic, fl::u8 octaveCount = 4)
            : type(noiseType),
              octaves(octaveCount) {
        }

        CartesianLayer layer() const override {
            return NoisePatternFunctor{type, octaves};
        }
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_CARTESIAN_CARTESIANNOISEPATTERN_H
