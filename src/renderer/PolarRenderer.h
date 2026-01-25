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

#ifndef POLAR_SHADER_POLAREFFECT_H
#define POLAR_SHADER_POLAREFFECT_H

#include <renderer/pipeline/units/Range.h>
#include <renderer/pipeline/PolarPipelineBuilder.h>

namespace PolarShader {
    using PolarCoords = fl::pair<FracQ0_16, FracQ0_16>;
    using PolarCoordsMapper = fl::function<PolarCoords(uint16_t pixelIndex)>;

    /**
     * PolarRenderer builds a PolarPipeline from the current palette and renders using 16-bit turn angles
     * (Q0.16). The pipeline assumes incoming noise layers already produce palette-ready intensities in
     * [0..65535]; no normalization occurs past the source layer.
     */
    class PolarRenderer {
        const PolarCoordsMapper coordsMapper;
        PolarPipeline pipeline;
        ColourLayer colourLayer;

    public:
        const uint16_t nbLeds;

        explicit PolarRenderer(
            uint16_t nbLeds,
            PolarCoordsMapper coordsMapper
        );

        void render(
            CRGB *outputArray,
            TimeMillis timeInMillis
        );
    };
} // namespace PolarShader

#endif //POLAR_SHADER_POLAREFFECT_H
