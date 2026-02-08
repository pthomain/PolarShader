//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#ifndef POLAR_SHADER_PIPELINE_SCENEPROVIDER_H
#define POLAR_SHADER_PIPELINE_SCENEPROVIDER_H

#include "Scene.h"
#include <memory>

namespace PolarShader {
    /**
     * @brief Interface for providing scenes to the SceneManager.
     */
    class SceneProvider {
    public:
        virtual ~SceneProvider() = default;

        /**
         * @brief Returns the next scene to play.
         */
        virtual std::unique_ptr<Scene> nextScene() = 0;
    };

    /**
     * @brief A simple provider that always returns the same scene (loops).
     */
    class DefaultSceneProvider : public SceneProvider {
        fl::function<std::unique_ptr<Scene>()> factory;

    public:
        explicit DefaultSceneProvider(fl::function<std::unique_ptr<Scene>()> factory)
            : factory(std::move(factory)) {}

        std::unique_ptr<Scene> nextScene() override {
            return factory();
        }
    };
}

#endif // POLAR_SHADER_PIPELINE_SCENEPROVIDER_H
