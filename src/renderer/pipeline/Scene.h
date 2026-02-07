//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#ifndef POLAR_SHADER_PIPELINE_SCENE_H
#define POLAR_SHADER_PIPELINE_SCENE_H

#include "Layer.h"
#include <vector>

namespace PolarShader {
    /**
     * @brief A Scene represents a collection of layers that are composited together.
     * 
     * It has a duration and manages relative timing for its layers.
     */
    class Scene {
        fl::vector<std::shared_ptr<Layer>> layers;
        TimeMillis durationMs;
        TimeMillis startTimeMs{0};

    public:
        Scene(fl::vector<std::shared_ptr<Layer>> layers, TimeMillis durationMs);

        void start(TimeMillis currentTimeMs);

        void advanceFrame(TimeMillis currentTimeMs);

        ColourMap build() const;

        TimeMillis getDuration() const { return durationMs; }
        
        bool isExpired(TimeMillis currentTimeMs) const;
    };
}

#endif // POLAR_SHADER_PIPELINE_SCENE_H
