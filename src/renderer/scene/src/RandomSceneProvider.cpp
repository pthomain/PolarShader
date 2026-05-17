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

#include "renderer/scene/RandomSceneProvider.h"
#include "renderer/pipeline/random/RandomEffectBuilder.h"

namespace PolarShader {
    std::unique_ptr<Scene> RandomSceneProvider::nextScene() {
        return random_fx::buildRandomScene(durationMs_);
    }
}
