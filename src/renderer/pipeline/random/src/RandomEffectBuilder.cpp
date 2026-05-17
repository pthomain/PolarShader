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

#include "renderer/pipeline/random/RandomEffectBuilder.h"

#if defined(ARDUINO) || defined(__EMSCRIPTEN__)
#include "FastLED.h"
#else
#include "native/FastLED.h"
#endif

#include "renderer/layer/Layer.h"
#include "renderer/layer/LayerBuilder.h"
#include "renderer/pipeline/patterns/Patterns.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/transforms/KaleidoscopeTransform.h"
#include "renderer/pipeline/transforms/PaletteTransform.h"
#include "renderer/pipeline/transforms/RadialKaleidoscopeTransform.h"
#include "renderer/pipeline/transforms/RotationTransform.h"
#include "renderer/pipeline/transforms/TilingTransform.h"
#include "renderer/pipeline/transforms/TranslationTransform.h"
#include "renderer/pipeline/transforms/VortexTransform.h"
#include "renderer/pipeline/transforms/ZoomTransform.h"

#include <utility>

namespace PolarShader::random_fx {
    namespace {
        // Inclusive [lo, hi] uniform random.
        uint16_t randInRange(uint16_t lo, uint16_t hi) {
            if (hi <= lo) return lo;
            return lo + random16(static_cast<uint16_t>(hi - lo + 1));
        }

        // Pick one of {constant, sine, triangle, noise}.
        // - constant: uniform permille in [0, 1000]
        // - waveforms: phase velocity (permille) in [pvMin, pvMax]
        Sf16Signal randomSignal(uint16_t pvMin = 30, uint16_t pvMax = 500) {
            uint8_t kind = random8(4);
            switch (kind) {
                case 0: {
                    uint16_t v = randInRange(0, 1000);
                    return constant(v);
                }
                case 1: return sine(constant(randInRange(pvMin, pvMax)));
                case 2: return triangle(constant(randInRange(pvMin, pvMax)));
                case 3:
                default: return noise(constant(randInRange(pvMin, pvMax)));
            }
        }

        std::unique_ptr<UVPattern> randomPattern() {
            // 9 entries; heavy grid sims (ReactionDiffusion, Flurry) excluded.
            uint8_t pick = random8(9);
            switch (pick) {
                case 0: return noisePattern(randomSignal());
                case 1: return fbmNoisePattern(static_cast<fl::u8>(randInRange(3, 5)));
                case 2: return turbulenceNoisePattern();
                case 3: return ridgedNoisePattern();
                case 4: {
                    uint16_t cellSize = randInRange(8, 64);
                    uint8_t  colors   = static_cast<uint8_t>(randInRange(3, 6));
                    auto shape = static_cast<TilingPattern::TileShape>(random8(3));
                    return tilingPattern(cellSize, colors, shape);
                }
                case 5: {
                    uint8_t arms = static_cast<uint8_t>(randInRange(2, 5));
                    bool cw = (random8() & 1) != 0;
                    return spiralPattern(arms, cw, randomSignal(), randomSignal(), randomSignal());
                }
                case 6: {
                    uint8_t rings  = static_cast<uint8_t>(randInRange(4, 12));
                    uint8_t slices = static_cast<uint8_t>(randInRange(16, 48));
                    bool reverse = (random8() & 1) != 0;
                    uint16_t stepMs = randInRange(40, 200);
                    uint16_t holdMs = randInRange(400, 1200);
                    return annuliPattern(rings, slices, reverse, stepMs, holdMs);
                }
                case 7: {
                    // Keep grid small (16) for MCU RAM.
                    auto mode = static_cast<TransportPattern::TransportMode>(random8(10));
                    bool glow = (random8() & 1) != 0;
                    return transportPattern(16, mode,
                                            randomSignal(), randomSignal(),
                                            randomSignal(), randomSignal(),
                                            glow);
                }
                case 8: {
                    int32_t cellRaw = static_cast<int32_t>(randInRange(4000, 16000));
                    auto alias = static_cast<WorleyAliasing>(random8(3));
                    fl::s24x8 cell = fl::s24x8::from_raw(cellRaw);
                    return (random8() & 1)
                               ? worleyPattern(cell, alias)
                               : voronoiPattern(cell, alias);
                }
                default: return noisePattern(randomSignal());
            }
        }

        void addRandomUVTransform(LayerBuilder &builder) {
            // 7 transform choices. FlowFieldTransform skipped (heavy on MCU).
            uint8_t pick = random8(7);
            switch (pick) {
                case 0: {
                    bool isTurn = (random8() & 1) != 0;
                    builder.addTransform(RotationTransform(randomSignal(), isTurn));
                    return;
                }
                case 1: {
                    builder.addTransform(ZoomTransform(randomSignal()));
                    return;
                }
                case 2: {
                    builder.addTransform(TranslationTransform(randomSignal(), randomSignal()));
                    return;
                }
                case 3: {
                    builder.addTransform(VortexTransform(randomSignal()));
                    return;
                }
                case 4: {
                    uint8_t facets = static_cast<uint8_t>(randInRange(2, 8));
                    bool mirror = (random8() & 1) != 0;
                    builder.addTransform(KaleidoscopeTransform(facets, mirror));
                    return;
                }
                case 5: {
                    uint16_t divs = randInRange(2, 8);
                    bool mirror = (random8() & 1) != 0;
                    builder.addTransform(RadialKaleidoscopeTransform(divs, mirror));
                    return;
                }
                case 6:
                default: {
                    bool mirror = (random8() & 1) != 0;
                    auto shape = static_cast<TilingMaths::TileShape>(random8(3));
                    builder.addTransform(TilingTransform(randomSignal(), mirror, shape));
                    return;
                }
            }
        }
    } // namespace

    std::unique_ptr<Scene> buildRandomScene(TimeMillis durationMs) {
        LayerBuilder builder(randomPattern(), CRGBPalette16(Rainbow_gp), "random");

        // Palette transform: animated offset (always).
        builder.addPaletteTransform(PaletteTransform(randomSignal()));

        // 3 or 4 random UV transforms.
        uint8_t count = static_cast<uint8_t>(3 + (random8() & 1));
        for (uint8_t i = 0; i < count; ++i) {
            addRandomUVTransform(builder);
        }

        fl::vector<std::shared_ptr<Layer>> layers;
        layers.push_back(std::make_shared<Layer>(builder.build()));
        return std::make_unique<Scene>(std::move(layers), durationMs);
    }
}
