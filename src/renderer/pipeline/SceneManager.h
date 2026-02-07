//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#ifndef POLAR_SHADER_PIPELINE_SCENEMANAGER_H
#define POLAR_SHADER_PIPELINE_SCENEMANAGER_H

#include "SceneProvider.h"
#include <memory>

namespace PolarShader {
    /**
     * @brief Manages the current scene and transitions to the next when expired.
     */
    class SceneManager {
        std::unique_ptr<SceneProvider> provider;
        std::unique_ptr<Scene> currentScene;
        ColourMap currentMap;

    public:
        explicit SceneManager(std::unique_ptr<SceneProvider> provider);

        void advanceFrame(TimeMillis currentTimeMs);

        ColourMap build() const;
    };
}

#endif // POLAR_SHADER_PIPELINE_SCENEMANAGER_H
