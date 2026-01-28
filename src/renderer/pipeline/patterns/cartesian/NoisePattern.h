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

            PatternNormU16 operator()(CartQ24_8 x, CartQ24_8 y) const {
                int64_t offset = static_cast<int64_t>(NOISE_DOMAIN_OFFSET) << CARTESIAN_FRAC_BITS;
                int64_t sx = static_cast<int64_t>(raw(x)) + offset;
                int64_t sy = static_cast<int64_t>(raw(y)) + offset;
                uint32_t ux = static_cast<uint32_t>(sx);
                uint32_t uy = static_cast<uint32_t>(sy);
                CartUQ24_8 xu(ux);
                CartUQ24_8 yu(uy);

                switch (type) {
                    case NoiseType::FBM:
                        return fBmLayerImpl(xu, yu, octaves);
                    case NoiseType::Turbulence:
                        return turbulenceLayerImpl(xu, yu);
                    case NoiseType::Ridged:
                        return ridgedLayerImpl(xu, yu);
                    case NoiseType::Basic:
                    default:
                        return noiseLayerImpl(xu, yu);
                }
            }
        };

        NoiseType type;
        fl::u8 octaves;

        static NoiseRawU16 sampleNoiseBilinear(uint32_t x, uint32_t y) {
            uint32_t x_int = x >> CARTESIAN_FRAC_BITS;
            uint32_t y_int = y >> CARTESIAN_FRAC_BITS;
            uint32_t frac_mask = (1u << CARTESIAN_FRAC_BITS) - 1u;
            uint16_t x_frac = static_cast<uint16_t>((x & frac_mask) << (16 - CARTESIAN_FRAC_BITS));
            uint16_t y_frac = static_cast<uint16_t>((y & frac_mask) << (16 - CARTESIAN_FRAC_BITS));

            uint16_t n00 = inoise16(x_int, y_int);
            uint16_t n10 = inoise16(x_int + 1u, y_int);
            uint16_t n01 = inoise16(x_int, y_int + 1u);
            uint16_t n11 = inoise16(x_int + 1u, y_int + 1u);

            int32_t nx0 = static_cast<int32_t>(n00) +
                          ((static_cast<int32_t>(n10) - static_cast<int32_t>(n00)) * x_frac >> 16);
            int32_t nx1 = static_cast<int32_t>(n01) +
                          ((static_cast<int32_t>(n11) - static_cast<int32_t>(n01)) * x_frac >> 16);
            int32_t nxy = nx0 + ((nx1 - nx0) * y_frac >> 16);
            if (nxy < 0) nxy = 0;
            if (nxy > UINT16_MAX) nxy = UINT16_MAX;
            return NoiseRawU16(nxy);
        }

        static PatternNormU16 noiseLayerImpl(CartUQ24_8 x, CartUQ24_8 y) {
            return noiseNormaliseU16(sampleNoiseBilinear(raw(x), raw(y)));
        }

        static PatternNormU16 fBmLayerImpl(CartUQ24_8 x, CartUQ24_8 y, fl::u8 octaveCount) {
            uint32_t r = 0;
            uint16_t amplitude = U16_HALF;
            for (int o = 0; o < octaveCount; o++) {
                auto n = sampleNoiseBilinear(raw(x), raw(y));
                r += (static_cast<uint32_t>(raw(n)) * amplitude) >> 16;
                x = CartUQ24_8(raw(x) << 1);
                y = CartUQ24_8(raw(y) << 1);
                amplitude >>= 1;
            }
            if (r > UINT16_MAX) r = UINT16_MAX;
            return noiseNormaliseU16(NoiseRawU16(static_cast<uint16_t>(r)));
        }

        static PatternNormU16 turbulenceLayerImpl(CartUQ24_8 x, CartUQ24_8 y) {
            NoiseRawU16 noise_raw = NoiseRawU16(sampleNoiseBilinear(raw(x), raw(y)));
            int16_t r = static_cast<int16_t>(raw(noise_raw)) - U16_HALF;
            uint16_t mag = static_cast<uint16_t>(r ^ (r >> 15)) - static_cast<uint16_t>(r >> 15);
            uint32_t doubled = static_cast<uint32_t>(mag) << 1;
            if (doubled > FRACT_Q0_16_MAX) doubled = FRACT_Q0_16_MAX;
            return noiseNormaliseU16(NoiseRawU16(static_cast<uint16_t>(doubled)));
        }

        static PatternNormU16 ridgedLayerImpl(CartUQ24_8 x, CartUQ24_8 y) {
            NoiseRawU16 noise_raw = NoiseRawU16(sampleNoiseBilinear(raw(x), raw(y)));
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
            : CartesianPattern(PatternKind::Noise, OutputSemantic::Field),
              type(noiseType),
              octaves(octaveCount) {
        }

        CartesianLayer layer() const override {
            return NoisePatternFunctor{type, octaves};
        }
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_CARTESIAN_CARTESIANNOISEPATTERN_H
