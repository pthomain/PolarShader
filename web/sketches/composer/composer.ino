// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2025 Pierre Thomain
//
// PolarShader composer demo. The browser's composer panel encodes a
// scene as a .psc binary blob (see SceneCodec.h for the wire format)
// and pushes it via composer_apply_scene(). The display geometry is a
// separate JS-driven concern via composer_set_display() — the .psc
// itself is display-agnostic.
//
// Display-switch lifecycle:
//   The active WebFastLedDisplay is destroyed and reconstructed for the
//   new geometry. FastLED's strip registry tolerates this because the web
//   display specs instantiate the SAME addLeds<WS2812, D1, GRB> template
//   specialisation — addLeds returns
//   a function-local static CLEDController that is re-pointed at the
//   new outputArray. (See FastLED.h:1093-1097 in the pinned commit.) If
//   a future spec uses a different chipset/pin/order, this assumption
//   would need revisiting.

#include <emscripten/emscripten.h>
#include <memory>
#include <vector>

#include "FabricDisplaySpec.h"
#include "Fabric32x8DisplaySpec.h"
#include "FibonacciDisplaySpec.h"
#include "Matrix128x128DisplaySpec.h"
#include "RoundDisplaySpec.h"
#include "display/WebDisplayGeometry.h"
#include "display/WebFastLedDisplay.h"
#include "composer/SceneCodec.h"

using namespace PolarShader;

namespace {
    constexpr uint8_t FABRIC_BRIGHTNESS = 255;
    constexpr uint8_t FABRIC32X8_BRIGHTNESS = 255;
    constexpr uint8_t SMARTMATRIX_BRIGHTNESS = 255;
    constexpr uint8_t ROUND_BRIGHTNESS = 255;
    constexpr uint8_t FIBONACCI_BRIGHTNESS = 255;
    constexpr uint8_t REFRESH_MS = 30;

    // Display tag values — must match composer_set_display(which) in JS.
    constexpr uint8_t DISPLAY_FABRIC       = 0;
    constexpr uint8_t DISPLAY_ROUND        = 1;
    constexpr uint8_t DISPLAY_FABRIC_32X8  = 2;
    constexpr uint8_t DISPLAY_SMARTMATRIX  = 3;
    constexpr uint8_t DISPLAY_FIBONACCI    = 4;

#ifndef COMPOSER_INITIAL_DISPLAY
#define COMPOSER_INITIAL_DISPLAY DISPLAY_FABRIC
#endif

    // Exactly one of these is non-null at a time. Holding both type slots
    // keeps the dispatch type-safe without runtime polymorphism.
    std::unique_ptr<WebFastLedDisplay<FabricDisplaySpec>>      fabricDisplay;
    std::unique_ptr<WebFastLedDisplay<Fabric32x8DisplaySpec>>  fabric32x8Display;
    std::unique_ptr<WebFastLedDisplay<Matrix128x128DisplaySpec>> smartMatrixDisplay;
    std::unique_ptr<WebFastLedDisplay<RoundDisplaySpec>>       roundDisplay;
    std::unique_ptr<WebFastLedDisplay<FibonacciDisplaySpec>>   fibonacciDisplay;
    uint8_t activeDisplay = COMPOSER_INITIAL_DISPLAY;

    // Most-recent successfully-decoded scene blob. Empty until the JS
    // side pushes its first valid scene; in that interim the display
    // shows PolarRenderer's built-in defaultPreset(Rainbow_gp) fallback.
    std::vector<uint8_t> lastValidBlob;

    void postComposerPhase(uint32_t seq, const char *phase) {
        EM_ASM({
            var phase = UTF8ToString($1);
            var message = 'seq ' + $0 + ' ' + phase;
            var inWorker = typeof WorkerGlobalScope !== 'undefined'
                && typeof self !== 'undefined'
                && self instanceof WorkerGlobalScope;
            if (inWorker && typeof postMessage === 'function') {
                postMessage({
                    type: 'debug_log',
                    payload: {
                        timestamp: (typeof performance !== 'undefined' ? performance.now().toFixed(1) + 'ms' : ''),
                        worker: true,
                        level: 'LOG',
                        module: 'COMPOSER',
                        message: message,
                        data: { seq: $0, phase: phase }
                    }
                });
            }
            if (typeof console !== 'undefined' && console.log) {
                console.log('[COMPOSER] ' + message);
            }
        }, seq, phase);
    }

    void buildActiveDisplay() {
        // FabricDisplaySpec / RoundDisplaySpec are stateless config
        // objects; instantiating them inline is cheap.
        if (activeDisplay == DISPLAY_ROUND) {
            fabricDisplay.reset();
            fabric32x8Display.reset();
            smartMatrixDisplay.reset();
            fibonacciDisplay.reset();
            static RoundDisplaySpec spec;
            static composer::DecodeStatus _ignored = composer::DecodeStatus::OK;
            (void) _ignored;
            // Geometry must outlive the display. We rebuild it each switch
            // because RoundDisplaySpec's params could in principle vary
            // across switches; in practice it's a defaulted object.
            static WebDisplayGeometry geometry = buildWebGeometry(spec);
            roundDisplay = std::make_unique<WebFastLedDisplay<RoundDisplaySpec>>(
                spec, geometry, ROUND_BRIGHTNESS, REFRESH_MS);
        } else if (activeDisplay == DISPLAY_FABRIC_32X8) {
            fabricDisplay.reset();
            smartMatrixDisplay.reset();
            roundDisplay.reset();
            fibonacciDisplay.reset();
            static Fabric32x8DisplaySpec spec;
            static WebDisplayGeometry geometry = buildWebGeometry(spec);
            fabric32x8Display = std::make_unique<WebFastLedDisplay<Fabric32x8DisplaySpec>>(
                spec, geometry, FABRIC32X8_BRIGHTNESS, REFRESH_MS);
        } else if (activeDisplay == DISPLAY_SMARTMATRIX) {
            fabricDisplay.reset();
            fabric32x8Display.reset();
            roundDisplay.reset();
            fibonacciDisplay.reset();
            static Matrix128x128DisplaySpec spec;
            static WebDisplayGeometry geometry = buildWebGeometry(spec);
            smartMatrixDisplay = std::make_unique<WebFastLedDisplay<Matrix128x128DisplaySpec>>(
                spec, geometry, SMARTMATRIX_BRIGHTNESS, REFRESH_MS);
        } else if (activeDisplay == DISPLAY_FIBONACCI) {
            fabricDisplay.reset();
            fabric32x8Display.reset();
            smartMatrixDisplay.reset();
            roundDisplay.reset();
            static FibonacciDisplaySpec spec;
            static WebDisplayGeometry geometry = buildWebGeometry(spec);
            fibonacciDisplay = std::make_unique<WebFastLedDisplay<FibonacciDisplaySpec>>(
                spec, geometry, FIBONACCI_BRIGHTNESS, REFRESH_MS);
        } else {
            fabric32x8Display.reset();
            smartMatrixDisplay.reset();
            roundDisplay.reset();
            fibonacciDisplay.reset();
            static FabricDisplaySpec spec;
            static WebDisplayGeometry geometry = buildWebGeometry(spec);
            fabricDisplay = std::make_unique<WebFastLedDisplay<FabricDisplaySpec>>(
                spec, geometry, FABRIC_BRIGHTNESS, REFRESH_MS);
        }
    }

    template<typename DISPLAY>
    void replaceSceneWithPhase(
        DISPLAY &display,
        std::unique_ptr<Scene> scene,
        bool preserveElapsed,
        uint32_t seq
    ) {
        postComposerPhase(seq, "compile-start");
        if (preserveElapsed) {
            display.replaceScenePreservingElapsedWithoutRender(std::move(scene));
        } else {
            display.replaceSceneWithoutRender(std::move(scene));
        }
        postComposerPhase(seq, "compile-end");
        postComposerPhase(seq, "render-start");
        display.renderNow();
        postComposerPhase(seq, "render-end");
    }

    void replaceActiveSceneWithPhase(std::unique_ptr<Scene> scene, bool preserveElapsed, uint32_t seq) {
        if (activeDisplay == DISPLAY_ROUND && roundDisplay) {
            replaceSceneWithPhase(*roundDisplay, std::move(scene), preserveElapsed, seq);
        } else if (activeDisplay == DISPLAY_FABRIC_32X8 && fabric32x8Display) {
            replaceSceneWithPhase(*fabric32x8Display, std::move(scene), preserveElapsed, seq);
        } else if (activeDisplay == DISPLAY_SMARTMATRIX && smartMatrixDisplay) {
            replaceSceneWithPhase(*smartMatrixDisplay, std::move(scene), preserveElapsed, seq);
        } else if (activeDisplay == DISPLAY_FIBONACCI && fibonacciDisplay) {
            replaceSceneWithPhase(*fibonacciDisplay, std::move(scene), preserveElapsed, seq);
        } else if (fabricDisplay) {
            replaceSceneWithPhase(*fabricDisplay, std::move(scene), preserveElapsed, seq);
        } else {
            postComposerPhase(seq, "replace-skip-no-display");
        }
    }

    // Decode `lastValidBlob` and push it through the active display's
    // renderer. No-op if the blob is empty (pre-first-apply state).
    void replayBlob(uint32_t seq) {
        if (lastValidBlob.empty()) {
            postComposerPhase(seq, "replay-skip-empty");
            return;
        }
        composer::DecodeStatus status;
        postComposerPhase(seq, "replay-decode-start");
        auto scene = composer::decodeScene(lastValidBlob.data(), lastValidBlob.size(), &status);
        if (!scene) {
            postComposerPhase(seq, "replay-decode-failed");
            return;  // shouldn't happen — the blob was validated on store
        }
        postComposerPhase(seq, "replay-decode-ok");
        replaceActiveSceneWithPhase(std::move(scene), false, seq);
    }

}

// ─────────────────────────────────────────────────────────────────────
// JS-callable endpoints
// ─────────────────────────────────────────────────────────────────────

extern "C" {

// Returns 0 on success, non-zero composer::DecodeStatus on failure.
// On failure: the active scene is NOT touched and the stored "last
// valid blob" is NOT overwritten — the previous scene keeps rendering.
EMSCRIPTEN_KEEPALIVE
int composer_apply_scene_seq(const uint8_t *bytes, uint32_t len, uint32_t seq) {
    postComposerPhase(seq, "apply-enter");
    composer::DecodeStatus status = composer::DecodeStatus::OK;
    postComposerPhase(seq, "decode-start");
    auto scene = composer::decodeScene(bytes, len, &status);
    if (!scene) {
        postComposerPhase(seq, "decode-failed");
        return static_cast<int>(status);
    }
    postComposerPhase(seq, "decode-ok");

    // Initial apply starts the scene clock. Later live edits keep elapsed
    // time so speed/phase changes affect the current frame instead of
    // restarting the animation at t=0.
    const bool firstApply = lastValidBlob.empty();
    replaceActiveSceneWithPhase(std::move(scene), !firstApply, seq);

    postComposerPhase(seq, "blob-store-start");
    lastValidBlob.assign(bytes, bytes + len);
    postComposerPhase(seq, "apply-ok");
    return 0;
}

EMSCRIPTEN_KEEPALIVE
int composer_apply_scene(const uint8_t *bytes, uint32_t len) {
    return composer_apply_scene_seq(bytes, len, 0);
}

// 0 = fabric, 1 = round, 2 = fabric 32x8, 3 = SmartMatrix 128x128, 4 = fibonacci. Rebuilds the display AND internally reapplies
// the most-recent valid scene blob — JS does not need to call
// composer_apply_scene after this. No-op if `which` matches the current
// display.
//
// Edge case — switch before any successful composer_apply_scene: the
// stored blob is empty, so no replaceScene call happens. The new
// display takes over and keeps rendering PolarRenderer's built-in
// defaultPreset(Rainbow_gp) fallback until the JS side pushes its
// first valid scene.
EMSCRIPTEN_KEEPALIVE
void composer_set_display_seq(uint8_t which, uint32_t seq) {
    postComposerPhase(seq, "display-enter");
    if (which != DISPLAY_FABRIC && which != DISPLAY_ROUND && which != DISPLAY_FABRIC_32X8 && which != DISPLAY_SMARTMATRIX && which != DISPLAY_FIBONACCI) {
        postComposerPhase(seq, "display-invalid");
        return;
    }
    if (which == activeDisplay && (fabricDisplay || fabric32x8Display || smartMatrixDisplay || roundDisplay || fibonacciDisplay)) {
        postComposerPhase(seq, "display-noop");
        return;
    }
    activeDisplay = which;
    postComposerPhase(seq, "display-build-start");
    buildActiveDisplay();
    postComposerPhase(seq, "display-build-end");
    postComposerPhase(seq, "display-replay-start");
    replayBlob(seq);
    postComposerPhase(seq, "display-replay-end");
    postComposerPhase(seq, "display-ok");
}

EMSCRIPTEN_KEEPALIVE
void composer_set_display(uint8_t which) {
    composer_set_display_seq(which, 0);
}

// 0 = fabric, 1 = round, 2 = fabric 32x8, 3 = SmartMatrix 128x128, 4 = fibonacci. Intended for generated JS to call before setup(),
// so the initial FastLED screen map matches the iframe URL.
EMSCRIPTEN_KEEPALIVE
void composer_set_initial_display(uint8_t which) {
    if (which != DISPLAY_FABRIC && which != DISPLAY_ROUND && which != DISPLAY_FABRIC_32X8 && which != DISPLAY_SMARTMATRIX && which != DISPLAY_FIBONACCI) return;
    if (fabricDisplay || fabric32x8Display || smartMatrixDisplay || roundDisplay || fibonacciDisplay) {
        composer_set_display(which);
        return;
    }
    activeDisplay = which;
}

EMSCRIPTEN_KEEPALIVE
uint8_t composer_get_display() {
    return activeDisplay;
}

}  // extern "C"

// ─────────────────────────────────────────────────────────────────────
// Arduino entry points
// ─────────────────────────────────────────────────────────────────────

void setup() {
    buildActiveDisplay();
}

void loop() {
    if (activeDisplay == DISPLAY_ROUND && roundDisplay) {
        roundDisplay->loop();
    } else if (activeDisplay == DISPLAY_FABRIC_32X8 && fabric32x8Display) {
        fabric32x8Display->loop();
    } else if (activeDisplay == DISPLAY_SMARTMATRIX && smartMatrixDisplay) {
        smartMatrixDisplay->loop();
    } else if (activeDisplay == DISPLAY_FIBONACCI && fibonacciDisplay) {
        fibonacciDisplay->loop();
    } else if (fabricDisplay) {
        fabricDisplay->loop();
    }
}
