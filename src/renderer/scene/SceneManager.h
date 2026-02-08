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

#ifndef POLAR_SHADER_PIPELINE_SCENEMANAGER_H
#define POLAR_SHADER_PIPELINE_SCENEMANAGER_H

#include "SceneProvider.h"
#include <memory>
#include "renderer/pipeline/transforms/base/Layers.h"

namespace PolarShader {
    /**
     * @brief Manages the high-level rendering lifecycle and scene transitions.
     * 
     * The SceneManager is responsible for:
     * 1. Fetching the next scene from a SceneProvider when the current one expires.
     * 2. Tracking the absolute time and calculating the time elapsed since the current scene started.
     * 3. Calculating the global normalized progress (0..1) of the current scene.
     * 4. Driving the animation of all layers within the active scene.
     */
    class SceneManager {
        std::unique_ptr<SceneProvider> provider;
        std::unique_ptr<Scene> currentScene;
        ColourMap currentMap;
        TimeMillis currentSceneStartTimeMs{0};

    public:
        explicit SceneManager(std::unique_ptr<SceneProvider> provider);

        void advanceFrame(TimeMillis currentTimeMs);

        ColourMap build() const;
    };
}

#endif // POLAR_SHADER_PIPELINE_SCENEMANAGER_H
