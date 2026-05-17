//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

/*
 * This file is part of PolarShader.
 *
 * PolarShader is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef POLAR_SHADER_PIPELINE_RANDOMSCENEPROVIDER_H
#define POLAR_SHADER_PIPELINE_RANDOMSCENEPROVIDER_H

#include "SceneProvider.h"
#include "renderer/pipeline/maths/units/Units.h"

namespace PolarShader {
    /**
     * Scene provider that emits a freshly-randomized Scene on every call to
     * nextScene(). Combined with SceneManager's expiry-driven swap, this
     * produces a new random animation every `durationMs` milliseconds.
     */
    class RandomSceneProvider : public SceneProvider {
        TimeMillis durationMs_;

    public:
        explicit RandomSceneProvider(TimeMillis durationMs = 30000) : durationMs_(durationMs) {}

        std::unique_ptr<Scene> nextScene() override;
    };
}

#endif // POLAR_SHADER_PIPELINE_RANDOMSCENEPROVIDER_H
