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
#include <renderer/pipeline/patterns/Patterns.h>
#include <renderer/scene/SceneManager.h>
#include <renderer/layer/Layer.h>

namespace PolarShader {
    PolarRenderer::PolarRenderer(
        uint16_t nbLeds,
        const PolarCoordsMapper& coordsMapper
    ) : sceneManager(std::make_unique<DefaultSceneProvider>([]() {
            fl::vector<std::shared_ptr<Layer> > layers;
            layers.push_back(std::make_shared<Layer>(defaultPreset(Rainbow_gp).build()));
            return std::make_unique<Scene>(std::move(layers));
        })),
        nbLeds(nbLeds) {
        precomputedCoords.reserve(nbLeds);
        for (uint16_t i = 0; i < nbLeds; ++i) {
            precomputedCoords.push_back(coordsMapper(i));
        }
    }

    void PolarRenderer::prepareFrame(TimeMillis timeInMillis) {
        sceneManager.advanceFrame(timeInMillis);
    }

    void PolarRenderer::render(
        CRGB *outputArray,
        TimeMillis timeInMillis
    ) {
        prepareFrame(timeInMillis);
        renderSlice(outputArray, 0, 1, 0);
    }

    void PolarRenderer::renderSlice(
        CRGB *outputArray,
        uint16_t start,
        uint16_t stride,
        uint8_t coreIndex
    ) const {
        for (uint16_t i = start; i < nbLeds; i += stride) {
            auto [angle, radius] = precomputedCoords[i];
            outputArray[i] = sceneManager.sample(coreIndex, angle, radius);
        }
    }
}
