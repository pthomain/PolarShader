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
//   new geometry. FastLED's strip registry tolerates this because both
//   FabricDisplaySpec and RoundDisplaySpec instantiate the SAME
//   addLeds<WS2812, D1, GRB> template specialisation — addLeds returns
//   a function-local static CLEDController that is re-pointed at the
//   new outputArray. (See FastLED.h:1093-1097 in the pinned commit.) If
//   a future spec uses a different chipset/pin/order, this assumption
//   would need revisiting.

#include <emscripten/emscripten.h>
#include <memory>
#include <vector>

#include "FabricDisplaySpec.h"
#include "RoundDisplaySpec.h"
#include "display/WebDisplayGeometry.h"
#include "display/WebFastLedDisplay.h"
#include "composer/SceneCodec.h"

using namespace PolarShader;

namespace {
    constexpr uint8_t BRIGHTNESS = 255;
    constexpr uint8_t REFRESH_MS = 30;

    // Display tag values — must match composer_set_display(which) in JS.
    constexpr uint8_t DISPLAY_FABRIC = 0;
    constexpr uint8_t DISPLAY_ROUND  = 1;

    // Exactly one of these is non-null at a time. Holding both type slots
    // keeps the dispatch type-safe without runtime polymorphism.
    std::unique_ptr<WebFastLedDisplay<FabricDisplaySpec>> fabricDisplay;
    std::unique_ptr<WebFastLedDisplay<RoundDisplaySpec>>  roundDisplay;
    uint8_t activeDisplay = DISPLAY_FABRIC;

    // Most-recent successfully-decoded scene blob. Empty until the JS
    // side pushes its first valid scene; in that interim the display
    // shows PolarRenderer's built-in defaultPreset(Rainbow_gp) fallback.
    std::vector<uint8_t> lastValidBlob;

    void buildActiveDisplay() {
        // FabricDisplaySpec / RoundDisplaySpec are stateless config
        // objects; instantiating them inline is cheap.
        if (activeDisplay == DISPLAY_ROUND) {
            fabricDisplay.reset();
            static RoundDisplaySpec spec;
            static composer::DecodeStatus _ignored = composer::DecodeStatus::OK;
            (void) _ignored;
            // Geometry must outlive the display. We rebuild it each switch
            // because RoundDisplaySpec's params could in principle vary
            // across switches; in practice it's a defaulted object.
            static WebDisplayGeometry geometry = buildWebGeometry(spec);
            roundDisplay = std::make_unique<WebFastLedDisplay<RoundDisplaySpec>>(
                spec, geometry, BRIGHTNESS, REFRESH_MS);
        } else {
            roundDisplay.reset();
            static FabricDisplaySpec spec;
            static WebDisplayGeometry geometry = buildWebGeometry(spec);
            fabricDisplay = std::make_unique<WebFastLedDisplay<FabricDisplaySpec>>(
                spec, geometry, BRIGHTNESS, REFRESH_MS);
        }
    }

    // Decode `lastValidBlob` and push it through the active display's
    // renderer. No-op if the blob is empty (pre-first-apply state).
    void replayBlob() {
        if (lastValidBlob.empty()) return;
        composer::DecodeStatus status;
        auto scene = composer::decodeScene(lastValidBlob.data(), lastValidBlob.size(), &status);
        if (!scene) return;  // shouldn't happen — the blob was validated on store
        if (activeDisplay == DISPLAY_ROUND && roundDisplay) {
            roundDisplay->replaceScene(std::move(scene));
        } else if (fabricDisplay) {
            fabricDisplay->replaceScene(std::move(scene));
        }
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
int composer_apply_scene(const uint8_t *bytes, uint32_t len) {
    composer::DecodeStatus status = composer::DecodeStatus::OK;
    auto scene = composer::decodeScene(bytes, len, &status);
    if (!scene) {
        return static_cast<int>(status);
    }

    // Push to the active renderer first; only then overwrite the cached
    // blob. Order doesn't strictly matter (decode already succeeded so
    // replay would also succeed), but this is clearer.
    if (activeDisplay == DISPLAY_ROUND && roundDisplay) {
        roundDisplay->replaceScene(std::move(scene));
    } else if (fabricDisplay) {
        fabricDisplay->replaceScene(std::move(scene));
    }

    lastValidBlob.assign(bytes, bytes + len);
    return 0;
}

// 0 = fabric, 1 = round. Rebuilds the display AND internally reapplies
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
void composer_set_display(uint8_t which) {
    if (which != DISPLAY_FABRIC && which != DISPLAY_ROUND) return;
    if (which == activeDisplay && (fabricDisplay || roundDisplay)) return;
    activeDisplay = which;
    buildActiveDisplay();
    replayBlob();
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
    } else if (fabricDisplay) {
        fabricDisplay->loop();
    }
}
