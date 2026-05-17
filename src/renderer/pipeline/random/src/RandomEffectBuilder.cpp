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
        // ───── Entropy injection ───────────────────────────────────────

        fl::vector<uint8_t> g_entropyPins;

        void injectAnalogEntropy() {
#if defined(ARDUINO) || defined(__EMSCRIPTEN__)
            if (g_entropyPins.empty()) return;
            uint8_t entropy = 0;
            for (uint8_t i = 0; i < 16; ++i) {
                entropy = static_cast<uint8_t>(
                    (entropy << 1) |
                    (analogRead(g_entropyPins[i % g_entropyPins.size()]) & 1)
                );
            }
            random16_add_entropy(entropy);
#endif
        }

        // ───── Primitive randomness helpers ────────────────────────────

        // Inclusive [lo, hi] uniform random. Uses uint32 math to support
        // the full uint16 range without overflow (e.g. lo=0, hi=0xFFFF).
        uint16_t randInRange(uint16_t lo, uint16_t hi) {
            if (hi <= lo) return lo;
            uint32_t span = static_cast<uint32_t>(hi) - static_cast<uint32_t>(lo) + 1u;
            if (span >= 0x10000u) return random16();
            return lo + random16(static_cast<uint16_t>(span));
        }

        // ───── Semantic signal helpers ─────────────────────────────────
        //
        // Each helper picks waveforms and phase-velocity ranges that match
        // the values used by hand-crafted presets in Presets.cpp. The goal
        // is to never produce strobing, never zoom to invisibility, never
        // max-twist a vortex, etc.

        // Static magnitude bounded away from the extremes that hide the
        // pattern (0 or 1000 permille).
        Sf16Signal slowConstant(uint16_t lo, uint16_t hi) {
            return constant(randInRange(lo, hi));
        }

        // Rotation speed (mapped through bipolarRange when isAngleTurn=false):
        // a slowly-modulated continuous rotation rate.
        Sf16Signal signalForRotationSpeed() {
            uint8_t kind = random8(3);
            switch (kind) {
                case 0:  return noise(constant(randInRange(50, 150)));
                case 1:  return sine(constant(randInRange(50, 150)));
                default: return slowConstant(400, 600); // small steady rate
            }
        }

        // Zoom scale: bounded so the pattern stays visible.
        // Magnitude 200..700 maps to roughly 1.6x..5.5x in ZoomTransform.
        Sf16Signal signalForZoom() {
            uint8_t kind = random8(3);
            switch (kind) {
                case 0:  return slowConstant(200, 700);
                case 1:  return sine(constant(randInRange(30, 100)));
                default: return noise(constant(randInRange(30, 100)),
                                       constant(200), constant(700));
            }
        }

        // Translation direction: a slow sweep around the full circle.
        Sf16Signal signalForTranslationDirection() {
            return noise(constant(randInRange(30, 100)));
        }

        // Translation speed: matches fabricPreset's bounded form (25..75
        // permille of max speed = ~6.25%..18.75% UV/frame ceiling).
        Sf16Signal signalForTranslationSpeed() {
            return noise(constant(randInRange(50, 100)),
                         constant(25), constant(75));
        }

        // Vortex strength: matches every preset's usage exactly.
        Sf16Signal signalForVortexStrength() {
            return noise(constant(100), constant(250), constant(750));
        }

        // Palette offset: a slow sweep so colours drift rather than flash.
        Sf16Signal signalForPaletteOffset() {
            return (random8() & 1)
                       ? noise(constant(randInRange(80, 200)))
                       : sine(constant(randInRange(80, 200)));
        }

        // Palette clip: bounded slow noise. Sampled in the unipolar magnitude
        // domain — 0 disables clipping, 1000 clips fully. Floor/ceiling stay
        // in the same range every preset uses (50..150 / 250..750) so the
        // clip amount drifts inside a moderate band rather than swinging
        // from no-clip to all-clip.
        Sf16Signal signalForPaletteClip() {
            // Random floor in [50, 300]; ceiling = floor + random width
            // (clamped at 950) so floor < ceiling holds.
            uint16_t floorPerMil = randInRange(50, 300);
            uint16_t ceilPerMil = floorPerMil + randInRange(150, 500);
            if (ceilPerMil > 950) ceilPerMil = 950;
            return noise(constant(randInRange(100, 300)),
                         constant(floorPerMil),
                         constant(ceilPerMil));
        }

        // Pattern-internal parameter (e.g. Transport radial speed, FlowField
        // amplitude). Most preset internal params are simple constants in
        // 200..800, with occasional slow noise.
        Sf16Signal signalForPatternParam() {
            uint8_t kind = random8(3);
            switch (kind) {
                case 0:  return slowConstant(200, 800);
                case 1:  return noise(constant(randInRange(50, 200)));
                default: return sine(constant(randInRange(50, 200)));
            }
        }

        // ───── Pattern selection ───────────────────────────────────────

        std::unique_ptr<UVPattern> randomPattern() {
            // 9 entries. Worley/Voronoi and TilingPattern (as a base) are
            // excluded — they don't appear in any preset and alias badly
            // under random params. ReactionDiffusion remains excluded for
            // MCU memory (it runs a full Gray-Scott grid simulation).
            uint8_t pick = random8(9);
            switch (pick) {
                case 0: return noisePattern(signalForPatternParam());
                case 1: return fbmNoisePattern(static_cast<fl::u8>(randInRange(3, 4)));
                case 2: return turbulenceNoisePattern();
                case 3: return ridgedNoisePattern();
                case 4: {
                    uint8_t arms = static_cast<uint8_t>(randInRange(2, 4));
                    bool cw = (random8() & 1) != 0;
                    return spiralPattern(arms, cw,
                                         signalForPatternParam(),
                                         signalForPatternParam(),
                                         signalForPatternParam());
                }
                case 5: {
                    uint8_t rings  = static_cast<uint8_t>(randInRange(6, 10));
                    uint8_t slices = static_cast<uint8_t>(randInRange(24, 32));
                    bool reverse = (random8() & 1) != 0;
                    uint16_t stepMs = randInRange(80, 160);
                    uint16_t holdMs = randInRange(600, 1000);
                    return annuliPattern(rings, slices, reverse, stepMs, holdMs);
                }
                case 6: {
                    auto mode = static_cast<TransportPattern::TransportMode>(random8(10));
                    bool glow = (random8() & 1) != 0;
                    return transportPattern(16, mode,
                                            signalForPatternParam(),
                                            signalForPatternParam(),
                                            signalForPatternParam(),
                                            signalForPatternParam(),
                                            glow);
                }
                case 7: {
                    uint8_t dots = static_cast<uint8_t>(randInRange(1, 3));
                    auto mode = static_cast<FlowFieldPattern::EmitterMode>(random8(3));
                    return flowFieldPattern(16, dots, mode,
                                            signalForPatternParam(),
                                            signalForPatternParam(),
                                            signalForPatternParam(),
                                            signalForPatternParam(),
                                            signalForPatternParam(),
                                            signalForPatternParam(),
                                            signalForPatternParam(),
                                            signalForPatternParam());
                }
                case 8:
                default: {
                    // Small grid (16) keeps Flurry's persistent scalar grid
                    // bounded on RP2040. lineCount stays low for the same
                    // reason and to keep emitters visually legible.
                    uint8_t lineCount = static_cast<uint8_t>(randInRange(1, 3));
                    auto shape = static_cast<FlurryPattern::Shape>(random8(2));
                    return flurryPattern(16, lineCount, shape,
                                         signalForPatternParam(),
                                         signalForPatternParam(),
                                         signalForPatternParam(),
                                         signalForPatternParam(),
                                         signalForPatternParam(),
                                         signalForPatternParam());
                }
            }
        }

        // ───── Modifier transforms (Zoom/Translation/Vortex) ───────────

        // Modifier identifiers; sampled without replacement in
        // addRandomEarlyModifiers.
        enum class Modifier : uint8_t { Zoom = 0, Translation = 1, Vortex = 2 };

        void applyModifier(LayerBuilder &builder, Modifier m) {
            switch (m) {
                case Modifier::Zoom:
                    builder.addTransform(ZoomTransform(signalForZoom()));
                    return;
                case Modifier::Translation:
                    builder.addTransform(TranslationTransform(
                        signalForTranslationDirection(),
                        signalForTranslationSpeed()));
                    return;
                case Modifier::Vortex:
                    builder.addTransform(VortexTransform(signalForVortexStrength()));
                    return;
            }
        }

        void addRandomEarlyModifiers(LayerBuilder &builder) {
            // Pick 1 or 2 modifiers without replacement. Fisher-Yates over
            // the 3-element pool.
            uint8_t count = static_cast<uint8_t>(1 + (random8() & 1));
            Modifier pool[3] = {Modifier::Zoom, Modifier::Translation, Modifier::Vortex};
            for (uint8_t i = 0; i < count; ++i) {
                uint8_t j = static_cast<uint8_t>(i + random8(static_cast<uint8_t>(3 - i)));
                Modifier tmp = pool[i]; pool[i] = pool[j]; pool[j] = tmp;
                applyModifier(builder, pool[i]);
            }
        }

        // ───── Geometry transforms ─────────────────────────────────────

        void addRandomGeometryTransform(LayerBuilder &builder) {
            static constexpr uint8_t kFacets[] = {2, 3, 4, 6};
            uint8_t roll = random8(10);
            bool mirror = (random8() & 1) != 0;
            if (roll < 5) {
                // Kaleidoscope (50%)
                uint8_t facets = kFacets[random8(4)];
                builder.addTransform(KaleidoscopeTransform(facets, mirror));
            } else if (roll < 8) {
                // RadialKaleidoscope (30%)
                uint16_t divs = kFacets[random8(4)];
                builder.addTransform(RadialKaleidoscopeTransform(divs, mirror));
            } else {
                // TilingTransform (20%): cell size in a pleasing range.
                auto shape = static_cast<TilingMaths::TileShape>(random8(3));
                builder.addTransform(TilingTransform(slowConstant(300, 600),
                                                     mirror, shape));
            }
        }
    } // namespace

    void setEntropyPins(fl::vector<uint8_t> pins) {
        g_entropyPins = std::move(pins);
#if defined(ARDUINO) || defined(__EMSCRIPTEN__)
        for (uint8_t p : g_entropyPins) pinMode(p, INPUT);
#endif
    }

    std::unique_ptr<Scene> buildRandomScene(TimeMillis durationMs) {
        // Stir fresh entropy into the FastLED PRNG before any random choice.
        injectAnalogEntropy();

        LayerBuilder builder(randomPattern(), CRGBPalette16(Rainbow_gp), "random");

        // 1. Palette transform — always. Offset + clip both driven by
        //    slow signals; maxFeather picked from the values seen in the
        //    hand-crafted presets (perMil 200..500).
        f16 maxFeather = perMil(randInRange(200, 500));
        builder.addPaletteTransform(PaletteTransform(
            signalForPaletteOffset(),
            signalForPaletteClip(),
            maxFeather));

        // 2. 1 or 2 early modifiers without replacement.
        addRandomEarlyModifiers(builder);

        // 3. Mandatory geometry transform.
        addRandomGeometryTransform(builder);

        // 4. Mandatory smooth rotation (isAngleTurn=false → continuous,
        //    accumulated rotation; no per-frame angle jumps).
        builder.addTransform(RotationTransform(signalForRotationSpeed(),
                                               /*isAngleTurn=*/false));

        fl::vector<std::shared_ptr<Layer>> layers;
        layers.push_back(std::make_shared<Layer>(builder.build()));
        return std::make_unique<Scene>(std::move(layers), durationMs);
    }
}
