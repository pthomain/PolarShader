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
#include "renderer/pipeline/signals/ranges/AngleRange.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/patterns/Patterns.h"
#include "renderer/pipeline/patterns/ConwayPattern.h"
#include "renderer/pipeline/patterns/ReactionDiffusionPattern.h"
#include "MatrixDisplaySpec.h"
#include "FabricDisplaySpec.h"
#include "RoundDisplaySpec.h"

#ifndef ARDUINO
#include "renderer/pipeline/maths/src/PolarMaths.cpp"
#include "renderer/pipeline/maths/src/NoiseMaths.cpp"
#include "renderer/pipeline/maths/src/PatternMaths.cpp"
#include "renderer/pipeline/maths/src/TimeMaths.cpp"
#include "renderer/pipeline/maths/src/TilingMaths.cpp"
#include "renderer/pipeline/signals/src/Signals.cpp"
#include "renderer/pipeline/signals/src/SignalSamplers.cpp"
#include "renderer/pipeline/signals/src/accumulators/Accumulators.cpp"
#include "renderer/pipeline/patterns/src/Patterns.cpp"
#include "renderer/pipeline/patterns/src/AnnuliPattern.cpp"
#include "renderer/pipeline/patterns/src/NoisePattern.cpp"
#include "renderer/pipeline/patterns/src/FlowFieldPattern.cpp"
#include "renderer/pipeline/patterns/src/FlurryPattern.cpp"
#include "renderer/pipeline/patterns/src/SpiralPattern.cpp"
#include "renderer/pipeline/patterns/src/TilingPattern.cpp"
#include "renderer/pipeline/patterns/src/TransportPattern.cpp"
#include "renderer/pipeline/patterns/src/ReactionDiffusionPattern.cpp"
#include "renderer/pipeline/patterns/src/ConwayPattern.cpp"
#include "renderer/pipeline/patterns/src/WorleyPatterns.cpp"
#include "renderer/pipeline/patterns/src/XORPattern.cpp"
#include "renderer/pipeline/patterns/src/base/UVPattern.cpp"
#include "renderer/layer/src/Layer.cpp"
#include "renderer/layer/src/LayerBuilder.cpp"
#include "renderer/scene/src/Scene.cpp"
#include "renderer/scene/src/SceneManager.cpp"
#endif

using namespace PolarShader;

void test_range_wraps_across_zero() {
    AngleRange range(f16(0xC000u), f16(0x4000u));
    TEST_ASSERT_EQUAL_UINT16(0xC000u, raw(range.map(sf16(SF16_MIN))));
    TEST_ASSERT_EQUAL_UINT16(0x0000u, raw(range.map(sf16(0))));
    TEST_ASSERT_EQUAL_UINT16(0x4000u, raw(range.map(sf16(SF16_MAX))));
}

// Global variable to capture progress
static f16 captured_progress(0);

namespace {
    class ProgressCapturePattern : public UVPattern {
    public:
        void advanceFrame(f16 progress, TimeMillis) override {
            captured_progress = progress;
        }
    };
}

void test_scene_progress_calculation() {
    // 1. Setup Layer with a pattern that captures progress in advanceFrame.
    auto layer = std::make_shared<Layer>(
        LayerBuilder(std::make_unique<ProgressCapturePattern>(), CloudColors_p, "TestLayer").build()
    );

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

void test_reaction_diffusion_compiled_sampler_tracks_front_buffer() {
    ReactionDiffusionPattern pattern(ReactionDiffusionPattern::Preset::Spots, 20, 20, 1);
    auto context = std::make_shared<PipelineContext>();
    uint16_t baseline[20][20];

    UVMap compiled = pattern.layer(context);
    for (uint8_t y = 0; y < 20; ++y) {
        for (uint8_t x = 0; x < 20; ++x) {
            UV probe(
                fl::s16x16::from_raw(static_cast<int32_t>((static_cast<uint32_t>(x) << 16) / 20u)),
                fl::s16x16::from_raw(static_cast<int32_t>((static_cast<uint32_t>(y) << 16) / 20u))
            );
            baseline[y][x] = raw(compiled(probe));
        }
    }

    pattern.advanceFrame(f16(0), 0);

    UVMap fresh = pattern.layer(context);
    bool foundChangedSample = false;

    for (uint8_t y = 0; y < 20 && !foundChangedSample; ++y) {
        for (uint8_t x = 0; x < 20; ++x) {
            UV probe(
                fl::s16x16::from_raw(static_cast<int32_t>((static_cast<uint32_t>(x) << 16) / 20u)),
                fl::s16x16::from_raw(static_cast<int32_t>((static_cast<uint32_t>(y) << 16) / 20u))
            );

            uint16_t compiledAfterAdvance = raw(compiled(probe));
            uint16_t freshAfterAdvance = raw(fresh(probe));

            TEST_ASSERT_EQUAL_UINT16(freshAfterAdvance, compiledAfterAdvance);
            if (baseline[y][x] != freshAfterAdvance) {
                foundChangedSample = true;
                break;
            }
        }
    }

    TEST_ASSERT_TRUE(foundChangedSample);
}

namespace {
    class TestMatrixSpec : public MatrixDisplaySpec {
        uint16_t w;
        uint16_t h;
    public:
        TestMatrixSpec(uint16_t width, uint16_t height) : w(width), h(height) {}
        uint16_t displayWidth() const override { return w; }
        uint16_t displayHeight() const override { return h; }
        uint16_t subsample() const override { return 1; }
    };

    std::shared_ptr<PipelineContext> rasterContext(uint16_t width, uint16_t height) {
        auto context = std::make_shared<PipelineContext>();
        context->rasterDisplay = RasterDisplayInfo{
            true,
            width,
            height,
            static_cast<uint32_t>(width) * height
        };
        return context;
    }

    RasterPoint rasterPoint(uint16_t x, uint16_t y, uint16_t width, uint16_t height) {
        return RasterPoint{
            true,
            static_cast<uint16_t>(static_cast<uint32_t>(y) * width + x),
            x,
            y,
            width,
            height
        };
    }

    void sampleRasterMap(const RasterMap &map, uint8_t *out, uint16_t width, uint16_t height) {
        for (uint16_t y = 0; y < height; ++y) {
            for (uint16_t x = 0; x < width; ++x) {
                out[static_cast<uint32_t>(y) * width + x] =
                    raw(map(rasterPoint(x, y, width, height))) != 0 ? 1u : 0u;
            }
        }
    }

    void sampleRasterColourMap(
        const RasterColourMap &map,
        uint8_t *cells,
        uint8_t *hues,
        uint16_t width,
        uint16_t height
    ) {
        for (uint16_t y = 0; y < height; ++y) {
            for (uint16_t x = 0; x < width; ++x) {
                const uint32_t idx = static_cast<uint32_t>(y) * width + x;
                const PaletteSample sample = map(rasterPoint(x, y, width, height));
                cells[idx] = raw(sample.value()) != 0 ? 1u : 0u;
                hues[idx] = static_cast<uint8_t>(raw(sample.hue()) >> 8);
            }
        }
    }

    void copyCells(uint8_t *dst, const uint8_t *src, uint32_t count) {
        for (uint32_t i = 0; i < count; ++i) {
            dst[i] = src[i];
        }
    }

    void stepCellsInPlace(uint8_t *current, uint8_t *scratch, uint16_t width, uint16_t height) {
        ConwayPattern::stepCells(current, scratch, width, height);
        copyCells(current, scratch, static_cast<uint32_t>(width) * height);
    }

    int16_t shortestTestHueDelta(uint8_t base, uint8_t hue) {
        int16_t delta = static_cast<int16_t>(hue) - static_cast<int16_t>(base);
        if (delta > 127) delta -= 256;
        if (delta < -128) delta += 256;
        return delta;
    }

    void accumulateTestNeighbourHue(
        const uint8_t *cells,
        const uint8_t *hues,
        uint32_t idx,
        uint8_t &hueCount,
        uint8_t &baseHue,
        int16_t &deltaSum
    ) {
        if (cells[idx] == 0) return;
        const uint8_t hue = hues[idx];
        if (hueCount == 0) {
            baseHue = hue;
        } else {
            deltaSum += shortestTestHueDelta(baseHue, hue);
        }
        ++hueCount;
    }

    uint8_t expectedBirthHue(
        const uint8_t *cells,
        const uint8_t *hues,
        uint16_t x,
        uint16_t y,
        uint16_t width,
        uint16_t height
    ) {
        const uint16_t yUp = y == 0 ? static_cast<uint16_t>(height - 1) : static_cast<uint16_t>(y - 1);
        const uint16_t yDown = y == height - 1 ? 0 : static_cast<uint16_t>(y + 1);
        const uint16_t xLeft = x == 0 ? static_cast<uint16_t>(width - 1) : static_cast<uint16_t>(x - 1);
        const uint16_t xRight = x == width - 1 ? 0 : static_cast<uint16_t>(x + 1);

        uint8_t hueCount = 0;
        uint8_t baseHue = hues[static_cast<uint32_t>(y) * width + x];
        int16_t deltaSum = 0;
        accumulateTestNeighbourHue(cells, hues, static_cast<uint32_t>(yUp) * width + xLeft,
                                   hueCount, baseHue, deltaSum);
        accumulateTestNeighbourHue(cells, hues, static_cast<uint32_t>(yUp) * width + x,
                                   hueCount, baseHue, deltaSum);
        accumulateTestNeighbourHue(cells, hues, static_cast<uint32_t>(yUp) * width + xRight,
                                   hueCount, baseHue, deltaSum);
        accumulateTestNeighbourHue(cells, hues, static_cast<uint32_t>(y) * width + xLeft,
                                   hueCount, baseHue, deltaSum);
        accumulateTestNeighbourHue(cells, hues, static_cast<uint32_t>(y) * width + xRight,
                                   hueCount, baseHue, deltaSum);
        accumulateTestNeighbourHue(cells, hues, static_cast<uint32_t>(yDown) * width + xLeft,
                                   hueCount, baseHue, deltaSum);
        accumulateTestNeighbourHue(cells, hues, static_cast<uint32_t>(yDown) * width + x,
                                   hueCount, baseHue, deltaSum);
        accumulateTestNeighbourHue(cells, hues, static_cast<uint32_t>(yDown) * width + xRight,
                                   hueCount, baseHue, deltaSum);
        return static_cast<uint8_t>(static_cast<int16_t>(baseHue) + (deltaSum / hueCount));
    }
}

void test_conway_step_rules() {
    const uint16_t W = 5;
    const uint16_t H = 5;
    uint8_t current[W * H] = {};
    uint8_t next[W * H] = {};

    current[2 * W + 1] = 1;
    current[2 * W + 2] = 1;
    current[2 * W + 3] = 1;

    ConwayPattern::stepCells(current, next, W, H);

    TEST_ASSERT_EQUAL_UINT8(1, next[1 * W + 2]);
    TEST_ASSERT_EQUAL_UINT8(1, next[2 * W + 2]);
    TEST_ASSERT_EQUAL_UINT8(1, next[3 * W + 2]);
    TEST_ASSERT_EQUAL_UINT8(0, next[2 * W + 1]);
    TEST_ASSERT_EQUAL_UINT8(0, next[2 * W + 3]);

    uint8_t single[W * H] = {};
    uint8_t singleNext[W * H] = {};
    single[2 * W + 2] = 1;
    ConwayPattern::stepCells(single, singleNext, W, H);
    TEST_ASSERT_EQUAL_UINT8(0, singleNext[2 * W + 2]);

    uint8_t block[W * H] = {};
    uint8_t blockNext[W * H] = {};
    block[1 * W + 1] = 1;
    block[1 * W + 2] = 1;
    block[2 * W + 1] = 1;
    block[2 * W + 2] = 1;
    ConwayPattern::stepCells(block, blockNext, W, H);
    TEST_ASSERT_EQUAL_UINT8(1, blockNext[1 * W + 1]);
    TEST_ASSERT_EQUAL_UINT8(1, blockNext[1 * W + 2]);
    TEST_ASSERT_EQUAL_UINT8(1, blockNext[2 * W + 1]);
    TEST_ASSERT_EQUAL_UINT8(1, blockNext[2 * W + 2]);
}

void test_conway_raster_layer_is_idempotent_and_deterministic() {
    auto context = rasterContext(4, 4);
    ConwayPattern pattern(250, 123, 500);
    RasterMap first = pattern.rasterLayer(context);
    RasterMap second = pattern.rasterLayer(context);
    ConwayPattern same(250, 123, 500);
    RasterMap sameMap = same.rasterLayer(context);

    for (uint16_t y = 0; y < 4; ++y) {
        for (uint16_t x = 0; x < 4; ++x) {
            RasterPoint point = rasterPoint(x, y, 4, 4);
            TEST_ASSERT_EQUAL_UINT16(raw(first(point)), raw(second(point)));
            TEST_ASSERT_EQUAL_UINT16(raw(first(point)), raw(sameMap(point)));
        }
    }
}

void test_conway_seed_layout_is_stable() {
    const uint16_t W = 5;
    const uint16_t H = 5;
    const uint8_t expected[W * H] = {
        0, 1, 1, 1, 1,
        1, 0, 0, 1, 0,
        0, 0, 0, 0, 0,
        1, 0, 0, 0, 0,
        1, 0, 0, 0, 0
    };
    auto context = rasterContext(W, H);
    ConwayPattern pattern(250, 1, 350);
    RasterMap map = pattern.rasterLayer(context);

    for (uint16_t y = 0; y < H; ++y) {
        for (uint16_t x = 0; x < W; ++x) {
            const uint32_t idx = static_cast<uint32_t>(y) * W + x;
            TEST_ASSERT_EQUAL_UINT8(
                expected[idx],
                raw(map(rasterPoint(x, y, W, H))) != 0 ? 1u : 0u
            );
        }
    }
}

void test_conway_seed_zero_uses_runtime_seed() {
    const uint16_t W = 5;
    const uint16_t H = 5;
    auto context = rasterContext(W, H);
    ConwayPattern first(250, 0, 350);
    ConwayPattern second(250, 0, 350);
    RasterMap firstMap = first.rasterLayer(context);
    RasterMap secondMap = second.rasterLayer(context);

    bool foundDifference = false;
    for (uint16_t y = 0; y < H; ++y) {
        for (uint16_t x = 0; x < W; ++x) {
            const bool a = raw(firstMap(rasterPoint(x, y, W, H))) != 0;
            const bool b = raw(secondMap(rasterPoint(x, y, W, H))) != 0;
            foundDifference = foundDifference || a != b;
        }
    }
    TEST_ASSERT_TRUE(foundDifference);
}

void test_conway_step_interval_uses_elapsed_time() {
    auto context = rasterContext(3, 3);
    ConwayPattern pattern(100, 7, 1000);
    RasterMap map = pattern.rasterLayer(context);
    RasterPoint center = rasterPoint(1, 1, 3, 3);

    TEST_ASSERT_EQUAL_UINT16(F16_MAX, raw(map(center)));
    pattern.advanceFrame(f16(0), 0);
    pattern.advanceFrame(f16(0), 99);
    TEST_ASSERT_EQUAL_UINT16(F16_MAX, raw(map(center)));
    pattern.advanceFrame(f16(0), 100);
    TEST_ASSERT_EQUAL_UINT16(0, raw(map(center)));
}

void test_conway_catch_up_is_capped() {
    const uint16_t W = 5;
    const uint16_t H = 5;
    auto context = rasterContext(W, H);
    ConwayPattern pattern(1, 1, 350);
    RasterMap map = pattern.rasterLayer(context);

    uint8_t expected[W * H] = {};
    uint8_t scratch[W * H] = {};
    sampleRasterMap(map, expected, W, H);

    pattern.advanceFrame(f16(0), 0);
    pattern.advanceFrame(f16(0), 6);

    for (uint8_t i = 0; i < 4; ++i) {
        stepCellsInPlace(expected, scratch, W, H);
    }

    for (uint16_t y = 0; y < H; ++y) {
        for (uint16_t x = 0; x < W; ++x) {
            TEST_ASSERT_EQUAL_UINT8(
                expected[static_cast<uint32_t>(y) * W + x],
                raw(map(rasterPoint(x, y, W, H))) != 0 ? 1u : 0u
            );
        }
    }
}

void test_conway_reseeds_when_static() {
    auto context = rasterContext(3, 3);
    ConwayPattern pattern(1, 1, 1000);
    RasterMap map = pattern.rasterLayer(context);
    RasterPoint center = rasterPoint(1, 1, 3, 3);

    TEST_ASSERT_EQUAL_UINT16(F16_MAX, raw(map(center)));
    pattern.advanceFrame(f16(0), 0);
    pattern.advanceFrame(f16(0), 1);
    TEST_ASSERT_EQUAL_UINT16(0, raw(map(center)));
    pattern.advanceFrame(f16(0), 2);
    TEST_ASSERT_EQUAL_UINT16(F16_MAX, raw(map(center)));
}

void test_conway_colour_survives_and_births_inherit() {
    const uint16_t W = 5;
    const uint16_t H = 5;
    auto context = rasterContext(W, H);
    ConwayPattern pattern(1, 1, 350);
    RasterColourMap map = pattern.rasterColourLayer(context);

    uint8_t initialCells[W * H] = {};
    uint8_t initialHues[W * H] = {};
    uint8_t expectedCells[W * H] = {};
    uint8_t scratch[W * H] = {};
    uint8_t actualCells[W * H] = {};
    uint8_t actualHues[W * H] = {};
    sampleRasterColourMap(map, initialCells, initialHues, W, H);
    copyCells(expectedCells, initialCells, W * H);
    stepCellsInPlace(expectedCells, scratch, W, H);

    pattern.advanceFrame(f16(0), 0);
    pattern.advanceFrame(f16(0), 1);
    sampleRasterColourMap(map, actualCells, actualHues, W, H);

    bool sawSurvivor = false;
    bool sawBirth = false;
    for (uint16_t y = 0; y < H; ++y) {
        for (uint16_t x = 0; x < W; ++x) {
            const uint32_t idx = static_cast<uint32_t>(y) * W + x;
            TEST_ASSERT_EQUAL_UINT8(expectedCells[idx], actualCells[idx]);
            if (!expectedCells[idx]) continue;

            if (initialCells[idx]) {
                sawSurvivor = true;
                TEST_ASSERT_EQUAL_UINT8(initialHues[idx], actualHues[idx]);
            } else {
                sawBirth = true;
                TEST_ASSERT_EQUAL_UINT8(expectedBirthHue(initialCells, initialHues, x, y, W, H), actualHues[idx]);
            }
        }
    }
    TEST_ASSERT_TRUE(sawSurvivor);
    TEST_ASSERT_TRUE(sawBirth);
}

void test_conway_invalid_and_over_capacity_raster_render_black() {
    ConwayPattern invalidPattern(250, 1, 1000);
    RasterMap invalidMap = invalidPattern.rasterLayer(std::make_shared<PipelineContext>());
    TEST_ASSERT_EQUAL_UINT16(0, raw(invalidMap(rasterPoint(0, 0, 1, 1))));

    auto tooLarge = rasterContext(65, 64);
    ConwayPattern capacityPattern(250, 1, 1000);
    RasterMap capacityMap = capacityPattern.rasterLayer(tooLarge);
    TEST_ASSERT_EQUAL_UINT16(0, raw(capacityMap(rasterPoint(0, 0, 65, 64))));
}

void test_display_specs_report_raster_points() {
    TestMatrixSpec matrix(3, 2);
    RenderPoint matrixPoint = matrix.toRenderPoint(4);
    TEST_ASSERT_TRUE(matrixPoint.raster.valid);
    TEST_ASSERT_EQUAL_UINT16(1, matrixPoint.raster.x);
    TEST_ASSERT_EQUAL_UINT16(1, matrixPoint.raster.y);
    TEST_ASSERT_EQUAL_UINT16(3, matrixPoint.raster.width);
    TEST_ASSERT_EQUAL_UINT16(2, matrixPoint.raster.height);

    FabricDisplaySpec fabric;
    RenderPoint fabricPoint = fabric.toRenderPoint(FabricDisplaySpec::WIDTH);
    PolarCoords fabricPolar = fabric.toPolarCoords(FabricDisplaySpec::WIDTH);
    TEST_ASSERT_TRUE(fabricPoint.raster.valid);
    TEST_ASSERT_EQUAL_UINT16(raw(fabricPolar.first), raw(fabricPoint.angle));
    TEST_ASSERT_EQUAL_UINT16(raw(fabricPolar.second), raw(fabricPoint.radius));
    TEST_ASSERT_EQUAL_UINT16(FabricDisplaySpec::WIDTH - 1, fabricPoint.raster.x);
    TEST_ASSERT_EQUAL_UINT16(1, fabricPoint.raster.y);
    TEST_ASSERT_EQUAL_UINT16((FabricDisplaySpec::WIDTH * 2) - 1, fabricPoint.raster.index);

    RoundDisplaySpec round;
    RenderPoint roundPoint = round.toRenderPoint(0);
    TEST_ASSERT_FALSE(roundPoint.raster.valid);
}

#ifdef ARDUINO

void setup() {

    UNITY_BEGIN();

    RUN_TEST(test_range_wraps_across_zero);
    RUN_TEST(test_scene_progress_calculation);
    RUN_TEST(test_scene_manager_lifecycle);
    RUN_TEST(test_reaction_diffusion_compiled_sampler_tracks_front_buffer);
    RUN_TEST(test_conway_step_rules);
    RUN_TEST(test_conway_raster_layer_is_idempotent_and_deterministic);
    RUN_TEST(test_conway_seed_layout_is_stable);
    RUN_TEST(test_conway_seed_zero_uses_runtime_seed);
    RUN_TEST(test_conway_step_interval_uses_elapsed_time);
    RUN_TEST(test_conway_catch_up_is_capped);
    RUN_TEST(test_conway_reseeds_when_static);
    RUN_TEST(test_conway_colour_survives_and_births_inherit);
    RUN_TEST(test_conway_invalid_and_over_capacity_raster_render_black);
    RUN_TEST(test_display_specs_report_raster_points);

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
    RUN_TEST(test_reaction_diffusion_compiled_sampler_tracks_front_buffer);
    RUN_TEST(test_conway_step_rules);
    RUN_TEST(test_conway_raster_layer_is_idempotent_and_deterministic);
    RUN_TEST(test_conway_seed_layout_is_stable);
    RUN_TEST(test_conway_seed_zero_uses_runtime_seed);
    RUN_TEST(test_conway_step_interval_uses_elapsed_time);
    RUN_TEST(test_conway_catch_up_is_capped);
    RUN_TEST(test_conway_reseeds_when_static);
    RUN_TEST(test_conway_colour_survives_and_births_inherit);
    RUN_TEST(test_conway_invalid_and_over_capacity_raster_render_black);
    RUN_TEST(test_display_specs_report_raster_points);

    return UNITY_END();

}

#endif
