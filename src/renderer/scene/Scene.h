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

#ifndef POLAR_SHADER_PIPELINE_SCENE_H
#define POLAR_SHADER_PIPELINE_SCENE_H

#include <renderer/layer/Layer.h>

namespace PolarShader {
    /**
     * @brief A Scene represents a collection of layers that are composited together.
     * 
     * A Scene has a fixed duration. It receives absolute time from the SceneManager 
     * and translates it to relative time (time since scene start). It also calculates 
     * the normalized progress (0.0 to 1.0) of the scene, which is passed to all 
     * contained layers for signal sampling.
     */
    class Scene {
        fl::vector<std::shared_ptr<Layer>> layers;
        TimeMillis durationMs;

    public:
        Scene(fl::vector<std::shared_ptr<Layer>> layers, TimeMillis durationMs = UINT32_MAX);

        void advanceFrame(FracQ0_16 progress, TimeMillis elapsedMs);

        ColourMap build() const;

        TimeMillis getDuration() const { return durationMs; }
        
        bool isExpired(TimeMillis elapsedMs) const;
    };
}

#endif // POLAR_SHADER_PIPELINE_SCENE_H
