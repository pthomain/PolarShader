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

#ifdef ARDUINO
#include <FastLED.h>
#else
#include "native/FastLED.h"
#endif
#include "renderer/pipeline/patterns/base/UVPattern.h"
#include "renderer/pipeline/maths/CartesianMaths.h"

namespace PolarShader {
    class NoisePattern : public UVPattern {
    public:
        enum class NoiseType {
            Basic,
            FBM,
            Turbulence,
            Ridged
        };

    private:
        struct UVNoisePatternFunctor;

        NoiseType type;
        fl::u8 octaves;

        static PatternNormU16 noiseLayerImpl(UQ24_8 x, UQ24_8 y, UQ24_8 z);

        static PatternNormU16 fBmLayerImpl(UQ24_8 x, UQ24_8 y, UQ24_8 z, fl::u8 octaveCount);

        static PatternNormU16 turbulenceLayerImpl(UQ24_8 x, UQ24_8 y, UQ24_8 z);

        static PatternNormU16 ridgedLayerImpl(UQ24_8 x, UQ24_8 y, UQ24_8 z);

    public:
        explicit NoisePattern(NoiseType noiseType = NoiseType::Basic, fl::u8 octaveCount = 4);

        UVMap layer(const std::shared_ptr<PipelineContext> &context) const override;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_CARTESIAN_CARTESIANNOISEPATTERN_H
