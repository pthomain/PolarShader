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

#if defined(ARDUINO) || defined(__EMSCRIPTEN__)
#include "FastLED.h"
#else
#include "native/FastLED.h"
#endif

namespace PolarShader::random_fx {
    /**
     * Build a randomly-configured Scene with the given duration.
     *
     * The scene is built around a fixed stack shape (mirroring the
     * hand-crafted presets):
     *   Palette -> 1 or 2 modifiers (Zoom/Translation/Vortex) ->
     *   Geometry (Kaleidoscope/RadialKaleidoscope/Tiling) -> Rotation.
     *
     * Signal value ranges are tuned per parameter to avoid extreme values
     * that would otherwise produce strobing, full-zoom-out, or full-vortex
     * artefacts. The palette is always Rainbow_gp.
     *
     * Uses FastLED random8()/random16() under the hood. Seeded once by
     * FastLedDisplay; per-scene re-injection is available via
     * setEntropyPins().
     */
    std::unique_ptr<Scene> buildRandomScene(TimeMillis durationMs = 30000);

    /**
     * Configure analog pins used to re-inject entropy into the FastLED PRNG
     * before every scene build. Empty list (default) disables. Idempotent.
     *
     * On native builds the pin list is stored but never read (no analogRead
     * stub), so tests remain deterministic.
     */
    void setEntropyPins(fl::vector<uint8_t> pins);
}

#endif // POLAR_SHADER_PIPELINE_RANDOM_RANDOMEFFECTBUILDER_H
