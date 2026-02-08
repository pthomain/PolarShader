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

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#include "native/FastLED.h"
#endif
#include <unity.h>
#include "renderer/pipeline/signals/ranges/PolarRange.h"
#include "renderer/pipeline/signals/ranges/LinearRange.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/patterns/Patterns.h"

#ifndef ARDUINO
#include "renderer/pipeline/maths/PolarMaths.cpp"
#include "renderer/pipeline/maths/NoiseMaths.cpp"
#include "renderer/pipeline/signals/Signals.cpp"
#include "renderer/pipeline/signals/SignalSamplers.cpp"
#include "renderer/pipeline/signals/Accumulators.cpp"
#include "renderer/pipeline/patterns/Patterns.cpp"
#include "renderer/pipeline/patterns/NoisePattern.cpp"
#include "renderer/pipeline/patterns/HexTilingPattern.cpp"
#include "renderer/pipeline/patterns/WorleyPatterns.cpp"
#include "renderer/pipeline/patterns/base/UVPattern.cpp"
#include "renderer/layer/Layer.cpp"
#include "renderer/layer/LayerBuilder.cpp"
#include "renderer/scene/Scene.cpp"
#include "renderer/scene/SceneManager.cpp"
#endif

using namespace PolarShader;

void test_range_wraps_across_zero() {
    PolarRange range(FracQ0_16(0xC000u), FracQ0_16(0x4000u));
    TEST_ASSERT_EQUAL_UINT16(0xC000u, raw(range.map(SFracQ0_16(Q0_16_MIN))));
    TEST_ASSERT_EQUAL_UINT16(0x0000u, raw(range.map(SFracQ0_16(0))));
    TEST_ASSERT_EQUAL_UINT16(0x3FFFu, raw(range.map(SFracQ0_16(Q0_16_MAX))));
}

// Global variable to capture progress
static FracQ0_16 captured_progress(0);

void test_scene_progress_calculation() {
    // 1. Setup Layer that captures progress
    auto layer = std::make_shared<Layer>(LayerBuilder(noisePattern(), CloudColors_p, "TestLayer")
        .withDepth([](FracQ0_16 progress, TimeMillis) {
            captured_progress = progress;
            return 0u;
        })
        .build());

    // 2. Setup Scene with 1000ms duration
    auto scene = std::make_unique<Scene>(
        fl::vector<std::shared_ptr<Layer>>{layer}, 
        1000 // durationMs
    );

    // 3. Setup SceneManager
    // We need a provider that returns this scene once.
    class SingleSceneProvider : public SceneProvider {
        std::unique_ptr<Scene> scene;
        bool returned = false;
    public:
        explicit SingleSceneProvider(std::unique_ptr<Scene> s) : scene(std::move(s)) {}
        std::unique_ptr<Scene> nextScene() override {
            if (!returned) {
                returned = true;
                return std::move(scene);
            }
            return nullptr; // Should loop or stop? SceneManager handles null by outputting black.
        }
    };
    
    auto provider = std::make_unique<SingleSceneProvider>(std::move(scene));
    SceneManager manager(std::move(provider));

    // 4. Test Progress
    
    // t=0: Progress should be 0
    manager.advanceFrame(0);
    TEST_ASSERT_EQUAL_UINT16(0, raw(captured_progress));

    // t=500: Progress should be 0.5 (0x8000 approx)
    // calculation: (500 * 65535) / 1000 = 32767.5 -> 32767
    manager.advanceFrame(500);
    // Allow small error due to integer math
    TEST_ASSERT_UINT16_WITHIN(10, 32767, raw(captured_progress));

    // t=999: Progress should be ~1.0
    manager.advanceFrame(999);
    TEST_ASSERT_UINT16_WITHIN(100, 0xFFFF, raw(captured_progress));
}

void test_scene_switching() {
    // Create two layers with distinct patterns to identify them.
    // We can use the 'blackLayer' or simple solid color patterns if we had a solid color pattern.
    // Instead, we can check side effects or use a provider that returns distinct scenes.
    
    // Let's rely on the SceneManager managing lifecycle.
    
    class DualSceneProvider : public SceneProvider {
        int count = 0;
    public:
        std::unique_ptr<Scene> nextScene() override {
            count++;
            // Scene 1: 500ms
            if (count == 1) {
                auto l = std::make_shared<Layer>(LayerBuilder(noisePattern(), CloudColors_p, "Scene1").build());
                return std::make_unique<Scene>(fl::vector<std::shared_ptr<Layer>>{l}, 500);
            }
            // Scene 2: 500ms
            if (count == 2) {
                auto l = std::make_shared<Layer>(LayerBuilder(noisePattern(), CloudColors_p, "Scene2").build());
                return std::make_unique<Scene>(fl::vector<std::shared_ptr<Layer>>{l}, 500);
            }
            return nullptr;
        }
    };

    auto provider = std::make_unique<DualSceneProvider>();
    SceneManager manager(std::move(provider));

    // t=0: Fetches Scene 1. Start time = 0.
    manager.advanceFrame(0);
    
    // t=400: Still Scene 1.
    manager.advanceFrame(400);
    // We can't easily check internal state, but we know nextScene() was called once.
    // If we had a way to check which scene is active...
    
    // t=550: Should switch to Scene 2.
    manager.advanceFrame(550);
    
    // How do we verify? 
    // Maybe checking if nextScene was called twice?
    // We can make the provider record calls.
}

// Improved provider for verification
static int provider_call_count = 0;
class TrackingSceneProvider : public SceneProvider {
public:
    std::unique_ptr<Scene> nextScene() override {
        provider_call_count++;
        auto l = std::make_shared<Layer>(LayerBuilder(noisePattern(), CloudColors_p, "Scene").build());
        return std::make_unique<Scene>(fl::vector<std::shared_ptr<Layer>>{l}, 100);
    }
};

void test_scene_manager_lifecycle() {
    provider_call_count = 0;
    auto provider = std::make_unique<TrackingSceneProvider>();
    SceneManager manager(std::move(provider));

    // t=0: Should call nextScene (count=1)
    manager.advanceFrame(0);
    TEST_ASSERT_EQUAL_INT(1, provider_call_count);

    // t=50: Still within 100ms duration. Count stays 1.
    manager.advanceFrame(50);
    TEST_ASSERT_EQUAL_INT(1, provider_call_count);

    // t=101: Expired. Should call nextScene (count=2).
    manager.advanceFrame(101);
    TEST_ASSERT_EQUAL_INT(2, provider_call_count);
}

#ifdef ARDUINO

void setup() {

    UNITY_BEGIN();

    RUN_TEST(test_range_wraps_across_zero);
    RUN_TEST(test_scene_progress_calculation);
    RUN_TEST(test_scene_manager_lifecycle);

    UNITY_END();

}



void loop() {

}

#else

int main(int argc, char **argv) {

    UNITY_BEGIN();

    RUN_TEST(test_range_wraps_across_zero);
    RUN_TEST(test_scene_progress_calculation);
    RUN_TEST(test_scene_manager_lifecycle);

    return UNITY_END();

}

#endif
