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
#ifndef ARDUINO
#include <algorithm>
#include <chrono>
#include <cstdio>
#endif
#include "renderer/pipeline/signals/ranges/AngleRange.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/patterns/Patterns.h"
#include "renderer/pipeline/patterns/ConwayPattern.h"
#include "renderer/pipeline/patterns/ReactionDiffusionPattern.h"
#include "MatrixDisplaySpec.h"
#include "Matrix128x128DisplaySpec.h"
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
#include "renderer/pipeline/patterns/src/PaletteGlowPattern.cpp"
#include "renderer/pipeline/patterns/src/SpiralPattern.cpp"
#include "renderer/pipeline/patterns/src/TilingPattern.cpp"
#include "renderer/pipeline/patterns/src/TransportPattern.cpp"
#include "renderer/pipeline/patterns/src/ReactionDiffusionPattern.cpp"
#include "renderer/pipeline/patterns/src/ConwayPattern.cpp"
#include "renderer/pipeline/patterns/src/CyclicCAPattern.cpp"
#include "renderer/pipeline/patterns/src/BriansBrainPattern.cpp"
#include "renderer/pipeline/patterns/src/LifeVariantPattern.cpp"
#include "renderer/pipeline/patterns/src/ElementaryCAPattern.cpp"
#include "renderer/pipeline/patterns/src/MatrixRainPattern.cpp"
#include "renderer/pipeline/patterns/src/RipplePattern.cpp"
#include "renderer/pipeline/patterns/src/ForestFirePattern.cpp"
#include "renderer/pipeline/patterns/src/WireWorldPattern.cpp"
#include "renderer/pipeline/patterns/src/LangtonAntPattern.cpp"
#include "renderer/pipeline/patterns/src/RasterReactionDiffusionPattern.cpp"
#include "renderer/pipeline/patterns/src/WorleyPatterns.cpp"
#include "renderer/pipeline/patterns/src/XORPattern.cpp"
#include "renderer/pipeline/patterns/src/base/UVPattern.cpp"
#include "renderer/pipeline/patterns/src/base/RasterAutomaton.cpp"
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

void test_palette_glow_pattern_emits_rgb_samples() {
    PaletteGlowPattern pattern;
    auto context = std::make_shared<PipelineContext>();
    pattern.advanceFrame(f16(0), 1000);

    UV probe(
        fl::s16x16::from_raw(0x8000),
        fl::s16x16::from_raw(0x8000)
    );
    UVLayer rgbLayer = pattern.uvLayer(context);
    UVMap scalar = pattern.layer(context);
    RgbSample sample = rgbLayer.rgb(probe);

    TEST_ASSERT_EQUAL_INT(static_cast<int>(UVLayerKind::Rgb), static_cast<int>(rgbLayer.kind));
    TEST_ASSERT_GREATER_THAN_UINT16(0, raw(sample.value()));
    TEST_ASSERT_EQUAL_UINT16(raw(sample.value()), raw(scalar(probe)));
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

namespace {
    uint8_t recoverCyclicState(uint16_t value, uint8_t numStates) {
        for (uint8_t s = 0; s < numStates; ++s) {
            const uint16_t expected =
                static_cast<uint16_t>((static_cast<uint32_t>(s) * F16_MAX) / (numStates - 1u));
            if (expected == value) return s;
        }
        return 0;
    }

    void assertRasterMapsEqual(const RasterMap &a, const RasterMap &b, uint16_t w, uint16_t h) {
        for (uint16_t y = 0; y < h; ++y) {
            for (uint16_t x = 0; x < w; ++x) {
                RasterPoint point = rasterPoint(x, y, w, h);
                TEST_ASSERT_EQUAL_UINT16(raw(a(point)), raw(b(point)));
            }
        }
    }
}

void test_raster_moore_neighbourhood_wraps() {
    // Toroidal Moore-neighbour counting shared by every cellular automaton.
    const uint8_t grid[9] = {
        5, 0, 5,
        0, 1, 0,
        5, 0, 5
    };
    // Centre sees the four corners (state 5) and four edges (state 0).
    TEST_ASSERT_EQUAL_UINT8(4, raster::countMooreState(grid, 1, 1, 3, 3, 5));
    TEST_ASSERT_EQUAL_UINT8(4, raster::countMooreState(grid, 1, 1, 3, 3, 0));
    // A corner wraps around the torus to see every other cell; three are 5.
    TEST_ASSERT_EQUAL_UINT8(3, raster::countMooreState(grid, 0, 0, 3, 3, 5));
}

void test_cyclic_ca_step_advances_on_threshold() {
    const uint16_t W = 8;
    const uint16_t H = 8;
    const uint8_t numStates = 4;
    const uint8_t threshold = 1;
    auto context = rasterContext(W, H);
    CyclicCAPattern pattern(120, 99, numStates, threshold);
    RasterMap map = pattern.rasterLayer(context);

    uint8_t initial[W * H];
    for (uint16_t y = 0; y < H; ++y) {
        for (uint16_t x = 0; x < W; ++x) {
            initial[y * W + x] =
                recoverCyclicState(raw(map(rasterPoint(x, y, W, H))), numStates);
        }
    }

    uint8_t expected[W * H];
    for (uint16_t y = 0; y < H; ++y) {
        for (uint16_t x = 0; x < W; ++x) {
            const uint32_t idx = static_cast<uint32_t>(y) * W + x;
            const uint8_t current = initial[idx];
            const uint8_t successor = static_cast<uint8_t>((current + 1u) % numStates);
            const uint8_t match = raster::countMooreState(initial, x, y, W, H, successor);
            expected[idx] = match >= threshold ? successor : current;
        }
    }

    pattern.advanceFrame(f16(0), 0);
    pattern.advanceFrame(f16(0), 120);

    for (uint16_t y = 0; y < H; ++y) {
        for (uint16_t x = 0; x < W; ++x) {
            const uint32_t idx = static_cast<uint32_t>(y) * W + x;
            TEST_ASSERT_EQUAL_UINT8(
                expected[idx],
                recoverCyclicState(raw(map(rasterPoint(x, y, W, H))), numStates)
            );
        }
    }
}

void test_cyclic_ca_is_deterministic() {
    auto context = rasterContext(6, 6);
    CyclicCAPattern first(120, 314, 6, 2);
    CyclicCAPattern second(120, 314, 6, 2);
    RasterMap firstMap = first.rasterLayer(context);
    RasterMap secondMap = second.rasterLayer(context);
    assertRasterMapsEqual(firstMap, secondMap, 6, 6);

    for (uint8_t i = 0; i < 3; ++i) {
        first.advanceFrame(f16(0), static_cast<TimeMillis>(120 * (i + 1)));
        second.advanceFrame(f16(0), static_cast<TimeMillis>(120 * (i + 1)));
    }
    assertRasterMapsEqual(firstMap, secondMap, 6, 6);
}

void test_brians_brain_cycles_firing_to_dying_to_off() {
    const uint16_t W = 8;
    const uint16_t H = 8;
    const uint16_t kFiring = F16_MAX;
    const uint16_t kDying = F16_MAX / 3u;
    auto context = rasterContext(W, H);
    BriansBrainPattern pattern(90, 77, 500);
    RasterMap map = pattern.rasterLayer(context);

    // Seeding produces only firing or off cells (never dying).
    bool initiallyFiring[W * H];
    bool sawFiring = false;
    for (uint16_t y = 0; y < H; ++y) {
        for (uint16_t x = 0; x < W; ++x) {
            const uint32_t idx = static_cast<uint32_t>(y) * W + x;
            const uint16_t value = raw(map(rasterPoint(x, y, W, H)));
            TEST_ASSERT_TRUE(value == kFiring || value == 0u);
            initiallyFiring[idx] = value == kFiring;
            sawFiring = sawFiring || initiallyFiring[idx];
        }
    }
    TEST_ASSERT_TRUE(sawFiring);

    // After one step every firing cell must have decayed to the dying state.
    pattern.advanceFrame(f16(0), 0);
    pattern.advanceFrame(f16(0), 90);
    for (uint16_t y = 0; y < H; ++y) {
        for (uint16_t x = 0; x < W; ++x) {
            const uint32_t idx = static_cast<uint32_t>(y) * W + x;
            if (initiallyFiring[idx]) {
                TEST_ASSERT_EQUAL_UINT16(kDying, raw(map(rasterPoint(x, y, W, H))));
            }
        }
    }

    // After a second step those dying cells must fall to off.
    pattern.advanceFrame(f16(0), 180);
    for (uint16_t y = 0; y < H; ++y) {
        for (uint16_t x = 0; x < W; ++x) {
            const uint32_t idx = static_cast<uint32_t>(y) * W + x;
            if (initiallyFiring[idx]) {
                TEST_ASSERT_EQUAL_UINT16(0u, raw(map(rasterPoint(x, y, W, H))));
            }
        }
    }
}

void test_life_seeds_rule_has_no_survivors() {
    const uint16_t W = 8;
    const uint16_t H = 8;
    auto context = rasterContext(W, H);
    LifeVariantPattern pattern(200, 55, 400, LifeVariantPattern::Rule::Seeds);
    RasterMap map = pattern.rasterLayer(context);

    bool initiallyAlive[W * H];
    bool sawAlive = false;
    for (uint16_t y = 0; y < H; ++y) {
        for (uint16_t x = 0; x < W; ++x) {
            const uint32_t idx = static_cast<uint32_t>(y) * W + x;
            initiallyAlive[idx] = raw(map(rasterPoint(x, y, W, H))) != 0;
            sawAlive = sawAlive || initiallyAlive[idx];
        }
    }
    TEST_ASSERT_TRUE(sawAlive);

    // Seeds has an empty survival set: no live cell can persist across a step.
    pattern.advanceFrame(f16(0), 0);
    pattern.advanceFrame(f16(0), 200);
    for (uint16_t y = 0; y < H; ++y) {
        for (uint16_t x = 0; x < W; ++x) {
            const uint32_t idx = static_cast<uint32_t>(y) * W + x;
            if (initiallyAlive[idx]) {
                TEST_ASSERT_EQUAL_UINT16(0u, raw(map(rasterPoint(x, y, W, H))));
            }
        }
    }
}

void test_life_highlife_birth_and_survival_masks() {
    const uint16_t W = 8;
    const uint16_t H = 8;
    // HighLife: B36/S23.
    const uint16_t birthMask = (1u << 3) | (1u << 6);
    const uint16_t survivalMask = (1u << 2) | (1u << 3);
    auto context = rasterContext(W, H);
    LifeVariantPattern pattern(200, 88, 350, LifeVariantPattern::Rule::HighLife);
    RasterMap map = pattern.rasterLayer(context);

    uint8_t initial[W * H];
    for (uint16_t y = 0; y < H; ++y) {
        for (uint16_t x = 0; x < W; ++x) {
            initial[y * W + x] = raw(map(rasterPoint(x, y, W, H))) != 0 ? 1u : 0u;
        }
    }

    uint8_t expected[W * H];
    for (uint16_t y = 0; y < H; ++y) {
        for (uint16_t x = 0; x < W; ++x) {
            const uint32_t idx = static_cast<uint32_t>(y) * W + x;
            const uint8_t neighbours = raster::countMooreState(initial, x, y, W, H, 1u);
            const uint16_t mask = initial[idx] ? survivalMask : birthMask;
            expected[idx] = (mask & (1u << neighbours)) != 0 ? 1u : 0u;
        }
    }

    pattern.advanceFrame(f16(0), 0);
    pattern.advanceFrame(f16(0), 200);

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

void test_elementary_ca_applies_rule_and_scrolls() {
    const uint16_t W = 8;
    const uint16_t H = 8;
    const uint8_t rule = 90;
    auto context = rasterContext(W, H);
    ElementaryCAPattern pattern(90, 33, rule);
    RasterMap map = pattern.rasterLayer(context);

    uint8_t initial[W * H];
    for (uint16_t y = 0; y < H; ++y) {
        for (uint16_t x = 0; x < W; ++x) {
            initial[y * W + x] = raw(map(rasterPoint(x, y, W, H))) != 0 ? 1u : 0u;
        }
    }

    // The bottom row advances by Wolfram's rule; every row then scrolls up one.
    const uint32_t bottom = static_cast<uint32_t>(H - 1) * W;
    uint8_t newRow[W];
    for (uint16_t x = 0; x < W; ++x) {
        const uint16_t xLeft = x == 0 ? static_cast<uint16_t>(W - 1) : static_cast<uint16_t>(x - 1);
        const uint16_t xRight = x == W - 1 ? 0 : static_cast<uint16_t>(x + 1);
        const uint8_t triple = static_cast<uint8_t>(
            (initial[bottom + xLeft] << 2) |
            (initial[bottom + x] << 1) |
            initial[bottom + xRight]);
        newRow[x] = static_cast<uint8_t>((rule >> triple) & 1u);
    }

    uint8_t expected[W * H];
    for (uint16_t y = 0; y + 1 < H; ++y) {
        for (uint16_t x = 0; x < W; ++x) {
            expected[y * W + x] = initial[(y + 1) * W + x];
        }
    }
    for (uint16_t x = 0; x < W; ++x) {
        expected[bottom + x] = newRow[x];
    }

    pattern.advanceFrame(f16(0), 0);
    pattern.advanceFrame(f16(0), 90);

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

void test_matrix_rain_heads_advance_and_light_up() {
    const uint16_t W = 8;
    const uint16_t H = 8;
    auto context = rasterContext(W, H);
    MatrixRainPattern pattern(60, 44, 40);
    RasterMap map = pattern.rasterLayer(context);

    // Freshly seeded: no cell is lit yet.
    for (uint16_t y = 0; y < H; ++y) {
        for (uint16_t x = 0; x < W; ++x) {
            TEST_ASSERT_EQUAL_UINT16(0u, raw(map(rasterPoint(x, y, W, H))));
        }
    }

    // One step advances every column head and lights exactly one cell per column.
    pattern.advanceFrame(f16(0), 0);
    pattern.advanceFrame(f16(0), 60);
    for (uint16_t x = 0; x < W; ++x) {
        uint8_t lit = 0;
        for (uint16_t y = 0; y < H; ++y) {
            if (raw(map(rasterPoint(x, y, W, H))) == F16_MAX) ++lit;
        }
        TEST_ASSERT_EQUAL_UINT8(1, lit);
    }
}

void test_matrix_rain_is_deterministic() {
    auto context = rasterContext(6, 6);
    MatrixRainPattern first(60, 271, 40);
    MatrixRainPattern second(60, 271, 40);
    RasterMap firstMap = first.rasterLayer(context);
    RasterMap secondMap = second.rasterLayer(context);
    for (uint8_t i = 0; i < 4; ++i) {
        first.advanceFrame(f16(0), static_cast<TimeMillis>(60 * (i + 1)));
        second.advanceFrame(f16(0), static_cast<TimeMillis>(60 * (i + 1)));
    }
    assertRasterMapsEqual(firstMap, secondMap, 6, 6);
}

void test_ripple_seeds_single_droplet() {
    const uint16_t W = 8;
    const uint16_t H = 8;
    auto context = rasterContext(W, H);
    RipplePattern pattern(40, 22, 6);
    RasterMap map = pattern.rasterLayer(context);

    // Seeding drops a single amplitude-4000 impulse (maps to 4000 << 4).
    uint16_t nonZero = 0;
    for (uint16_t y = 0; y < H; ++y) {
        for (uint16_t x = 0; x < W; ++x) {
            const uint16_t value = raw(map(rasterPoint(x, y, W, H)));
            if (value != 0) {
                ++nonZero;
                TEST_ASSERT_EQUAL_UINT16(static_cast<uint16_t>(4000u << 4), value);
            }
        }
    }
    TEST_ASSERT_EQUAL_UINT16(1, nonZero);
}

void test_ripple_is_deterministic() {
    auto context = rasterContext(6, 6);
    RipplePattern first(40, 909, 6);
    RipplePattern second(40, 909, 6);
    RasterMap firstMap = first.rasterLayer(context);
    RasterMap secondMap = second.rasterLayer(context);
    for (uint8_t i = 0; i < 5; ++i) {
        first.advanceFrame(f16(0), static_cast<TimeMillis>(40 * (i + 1)));
        second.advanceFrame(f16(0), static_cast<TimeMillis>(40 * (i + 1)));
    }
    assertRasterMapsEqual(firstMap, secondMap, 6, 6);
}

#ifndef ARDUINO
void test_palette_glow_rgb_1000_frame_perf_guard() {
    constexpr uint16_t width = Matrix128x128DisplaySpec::DISPLAY_WIDTH;
    constexpr uint16_t height = Matrix128x128DisplaySpec::DISPLAY_HEIGHT;
    constexpr uint16_t frames = 1000;
    constexpr uint32_t refreshBudgetUs = 30000u;

    Layer layer = LayerBuilder(std::make_unique<PaletteGlowPattern>(), CloudColors_p, "rgb-glow-perf").build();
    layer.setRasterDisplayInfo(RasterDisplayInfo{true, width, height, static_cast<uint32_t>(width) * height});
    auto map = layer.compile();
    TEST_ASSERT_NOT_NULL(map.get());

    uint32_t frameUs[frames] = {};
    volatile uint32_t sink = 0;

    for (uint16_t frame = 0; frame < frames; ++frame) {
        layer.advanceFrame(f16(0), static_cast<TimeMillis>(frame) * 16u);
        auto start = std::chrono::steady_clock::now();
        for (uint16_t y = 0; y < height; ++y) {
            uint16_t radius = static_cast<uint16_t>((static_cast<uint32_t>(y) * F16_MAX) / (height - 1u));
            for (uint16_t x = 0; x < width; ++x) {
                uint16_t angle = static_cast<uint16_t>((static_cast<uint32_t>(x) * F16_MAX) / width);
                CRGB color = (*map)(RenderPoint{f16(angle), f16(radius), RasterPoint{}});
                sink += static_cast<uint32_t>(color.r) + color.g + color.b;
            }
        }
        auto end = std::chrono::steady_clock::now();
        uint64_t elapsedUs = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()
        );
        frameUs[frame] = elapsedUs > UINT32_MAX ? UINT32_MAX : static_cast<uint32_t>(elapsedUs);
    }

    std::sort(frameUs, frameUs + frames);
    uint32_t totalUs = 0;
    for (uint16_t i = 0; i < frames; ++i) totalUs += frameUs[i];
    uint32_t avgUs = totalUs / frames;
    uint32_t p95Us = frameUs[(frames * 95u) / 100u];
    std::printf("PaletteGlow RGB perf: avg=%luus p95=%luus sink=%lu\n",
                static_cast<unsigned long>(avgUs),
                static_cast<unsigned long>(p95Us),
                static_cast<unsigned long>(sink));

    TEST_ASSERT_LESS_THAN_UINT32(refreshBudgetUs, p95Us);
}
#endif

#ifdef ARDUINO

void setup() {

    UNITY_BEGIN();

    RUN_TEST(test_range_wraps_across_zero);
    RUN_TEST(test_scene_progress_calculation);
    RUN_TEST(test_scene_manager_lifecycle);
    RUN_TEST(test_palette_glow_pattern_emits_rgb_samples);
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
    RUN_TEST(test_raster_moore_neighbourhood_wraps);
    RUN_TEST(test_cyclic_ca_step_advances_on_threshold);
    RUN_TEST(test_cyclic_ca_is_deterministic);
    RUN_TEST(test_brians_brain_cycles_firing_to_dying_to_off);
    RUN_TEST(test_life_seeds_rule_has_no_survivors);
    RUN_TEST(test_life_highlife_birth_and_survival_masks);
    RUN_TEST(test_elementary_ca_applies_rule_and_scrolls);
    RUN_TEST(test_matrix_rain_heads_advance_and_light_up);
    RUN_TEST(test_matrix_rain_is_deterministic);
    RUN_TEST(test_ripple_seeds_single_droplet);
    RUN_TEST(test_ripple_is_deterministic);
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
    RUN_TEST(test_palette_glow_pattern_emits_rgb_samples);
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
    RUN_TEST(test_raster_moore_neighbourhood_wraps);
    RUN_TEST(test_cyclic_ca_step_advances_on_threshold);
    RUN_TEST(test_cyclic_ca_is_deterministic);
    RUN_TEST(test_brians_brain_cycles_firing_to_dying_to_off);
    RUN_TEST(test_life_seeds_rule_has_no_survivors);
    RUN_TEST(test_life_highlife_birth_and_survival_masks);
    RUN_TEST(test_elementary_ca_applies_rule_and_scrolls);
    RUN_TEST(test_matrix_rain_heads_advance_and_light_up);
    RUN_TEST(test_matrix_rain_is_deterministic);
    RUN_TEST(test_ripple_seeds_single_droplet);
    RUN_TEST(test_ripple_is_deterministic);
    RUN_TEST(test_display_specs_report_raster_points);
    RUN_TEST(test_palette_glow_rgb_1000_frame_perf_guard);

    return UNITY_END();

}

#endif
