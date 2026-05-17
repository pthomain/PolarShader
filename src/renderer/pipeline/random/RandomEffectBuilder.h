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

#ifndef POLAR_SHADER_PIPELINE_RANDOM_RANDOMEFFECTBUILDER_H
#define POLAR_SHADER_PIPELINE_RANDOM_RANDOMEFFECTBUILDER_H

#include <memory>
#include "renderer/scene/Scene.h"
#include "renderer/pipeline/maths/units/Units.h"

namespace PolarShader::random_fx {
    /**
     * Build a randomly-configured Scene with the given duration.
     *
     * The scene contains one Layer with:
     *   - a randomly-picked pattern (excludes heavy grid sims),
     *   - a PaletteTransform driven by a random signal,
     *   - 3 or 4 random UV transforms, each driven by random signals.
     *
     * The palette is always Rainbow_gp.
     *
     * Uses FastLED random8()/random16() — seed externally before calling.
     */
    std::unique_ptr<Scene> buildRandomScene(TimeMillis durationMs = 30000);
}

#endif // POLAR_SHADER_PIPELINE_RANDOM_RANDOMEFFECTBUILDER_H
