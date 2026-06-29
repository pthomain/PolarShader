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

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#include "native/FastLED.h"
#endif
#include <unity.h>

#include "renderer/pipeline/random/RandomEffectBuilder.h"
#include "renderer/scene/Scene.h"

#ifndef ARDUINO
// Pull in source files directly because the native test env does not link
// the project's src/ tree. Matches the pattern used by test_pipeline.cpp.
#include "renderer/pipeline/maths/src/PolarMaths.cpp"
#include "renderer/pipeline/maths/src/NoiseMaths.cpp"
#include "renderer/pipeline/maths/src/PatternMaths.cpp"
#include "renderer/pipeline/maths/src/TimeMaths.cpp"
#include "renderer/pipeline/maths/src/TilingMaths.cpp"
#include "renderer/pipeline/signals/src/Signals.cpp"
#include "renderer/pipeline/signals/src/SignalSamplers.cpp"
#include "renderer/pipeline/signals/src/accumulators/Accumulators.cpp"
#include "renderer/pipeline/patterns/src/Patterns.cpp"
#include "renderer/pipeline/patterns/src/NoisePattern.cpp"
#include "renderer/pipeline/patterns/src/TilingPattern.cpp"
#include "renderer/pipeline/patterns/src/WorleyPatterns.cpp"
#include "renderer/pipeline/patterns/src/SpiralPattern.cpp"
#include "renderer/pipeline/patterns/src/AnnuliPattern.cpp"
#include "renderer/pipeline/patterns/src/TransportPattern.cpp"
#include "renderer/pipeline/patterns/src/FlowFieldPattern.cpp"
#include "renderer/pipeline/patterns/src/FlurryPattern.cpp"
#include "renderer/pipeline/patterns/src/ReactionDiffusionPattern.cpp"
#include "renderer/pipeline/patterns/src/base/UVPattern.cpp"
#include "renderer/pipeline/transforms/src/RotationTransform.cpp"
#include "renderer/pipeline/transforms/src/ZoomTransform.cpp"
#include "renderer/pipeline/transforms/src/TranslationTransform.cpp"
#include "renderer/pipeline/transforms/src/VortexTransform.cpp"
#include "renderer/pipeline/transforms/src/KaleidoscopeTransform.cpp"
#include "renderer/pipeline/transforms/src/RadialKaleidoscopeTransform.cpp"
#include "renderer/pipeline/transforms/src/TilingTransform.cpp"
#include "renderer/pipeline/transforms/src/PaletteTransform.cpp"
#include "renderer/pipeline/transforms/src/FlowFieldTransform.cpp"
#include "renderer/layer/src/Layer.cpp"
#include "renderer/layer/src/LayerBuilder.cpp"
#include "renderer/scene/src/Scene.cpp"
#include "renderer/scene/src/SceneManager.cpp"
#include "renderer/pipeline/random/src/RandomEffectBuilder.cpp"
#endif

using namespace PolarShader;

// Builds 50 random scenes, compiles each, samples a handful of points.
// Verifies no nullptr returns and no crashes across many random configurations.
void test_random_scene_builds_and_samples() {
    constexpr int kIterations = 50;
    constexpr TimeMillis kDuration = 30000;

    for (int i = 0; i < kIterations; ++i) {
        auto scene = random_fx::buildRandomScene(kDuration);
        TEST_ASSERT_NOT_NULL(scene.get());
        TEST_ASSERT_EQUAL_UINT32(kDuration, scene->getDuration());

        scene->compile();
        scene->advanceFrame(f16(0), 0);

        // Sample a few polar coordinates; mainly checks for crashes.
        (void) scene->sample(0, f16(0x0000), f16(0x0000));
        (void) scene->sample(0, f16(0x4000), f16(0x8000));
        (void) scene->sample(0, f16(0x8000), f16(0xFFFF));
    }
}

#ifdef ARDUINO
void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_random_scene_builds_and_samples);
    UNITY_END();
}

void loop() {}
#else
int main(int, char **) {
    UNITY_BEGIN();
    RUN_TEST(test_random_scene_builds_and_samples);
    return UNITY_END();
}
#endif
