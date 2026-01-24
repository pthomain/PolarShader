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

#include "PolarRenderer.h"
#include <utility>
#include <renderer/pipeline/presets/Presets.h>

namespace PolarShader {
    PolarRenderer::PolarRenderer(
        uint16_t nbLeds,
        PolarCoordsMapper coordsMapper
    ) : coordsMapper(std::move(coordsMapper)),
        pipeline(buildBarrelTunnelPreset(Rainbow_gp)),
        colourLayer(pipeline.build()),
        nbLeds(nbLeds) {
    }

    void PolarRenderer::render(
        CRGB *outputArray,
        TimeMillis timeInMillis
    ) {
        pipeline.advanceFrame(timeInMillis);
        for (uint16_t pixelIndex = 0; pixelIndex < nbLeds; ++pixelIndex) {
            auto [angle, radius] = coordsMapper(pixelIndex);
            outputArray[pixelIndex] = colourLayer(angle, radius);
        }
    }
}
