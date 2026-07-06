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
#endif
#include <unity.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "composer/EmbeddedPscPlaylist.h"
#include "composer/SceneCodec.h"
#include "composer/PaletteTable.h"
#include "renderer/scene/Scene.h"
#include "renderer/layer/Layer.h"
#include "renderer/layer/LayerBuilder.h"
#include "renderer/pipeline/patterns/Patterns.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/transforms/RotationTransform.h"
#include "renderer/pipeline/transforms/ZoomTransform.h"
#include "renderer/pipeline/transforms/KaleidoscopeTransform.h"
#include "renderer/pipeline/transforms/PaletteTransform.h"

using namespace PolarShader;
using namespace PolarShader::composer;

namespace {

    // ───── Wire-byte builder ──────────────────────────────────────────
    //
    // Mirrors the SIG_*/PAT_*/TFM_* tag tables in SceneCodec.cpp. Tests
    // author bytes directly so the wire format is visible inline (no risk
    // of an in-process encoder hiding a schema bug from the decoder).

    enum : uint8_t {
        SIG_CONSTANT          = 0x00,
        SIG_C_RANDOM          = 0x01,
        SIG_LINEAR            = 0x02,
        SIG_QUADRATIC_IN      = 0x03,
        SIG_QUADRATIC_OUT     = 0x04,
        SIG_QUADRATIC_IN_OUT  = 0x05,
        SIG_SINE              = 0x10,
        SIG_SINE_BOUNDED      = 0x11,
        SIG_SINE_BOUNDED_PH   = 0x12,
        SIG_TRIANGLE          = 0x13,
        SIG_TRIANGLE_BOUNDED  = 0x14,
        SIG_TRIANGLE_BOUNDED_PH = 0x15,
        SIG_SQUARE            = 0x16,
        SIG_SQUARE_BOUNDED    = 0x17,
        SIG_SQUARE_BOUNDED_PH = 0x18,
        SIG_SAWTOOTH          = 0x19,
        SIG_SAWTOOTH_BOUNDED  = 0x1A,
        SIG_SAWTOOTH_BOUNDED_PH = 0x1B,
        SIG_NOISE             = 0x1C,
        SIG_NOISE_BOUNDED     = 0x1D,
        SIG_NOISE_BOUNDED_PH  = 0x1E,
        SIG_SMAP              = 0x1F,
        SIG_SCALE             = 0x20,
    };

    enum : uint8_t {
        PAT_NOISE_BASIC       = 0x00,
        PAT_NOISE_FBM         = 0x01,
        PAT_NOISE_TURBULENCE  = 0x02,
        PAT_NOISE_RIDGED      = 0x03,
        PAT_TILING            = 0x04,
        PAT_REACTION_DIFFUSION= 0x05,
        PAT_FLOW_FIELD        = 0x06,
        PAT_TRANSPORT         = 0x07,
        PAT_SPIRAL            = 0x08,
        PAT_ANNULI            = 0x09,
        PAT_FLURRY            = 0x0A,
        PAT_WORLEY            = 0x0B,
        PAT_VORONOI           = 0x0C,
        PAT_PF_DUAL_AXIS      = 0x0D,
        PAT_PF_COUNTER_RIBBONS= 0x0E,
        PAT_PF_QUAD_DIRECTIONAL = 0x0F,
        PAT_PF_POSTERIZED     = 0x10,
        PAT_PF_CROSS          = 0x11,
        PAT_PF_PETALS         = 0x12,
        PAT_PF_RIPPLE         = 0x13,
        PAT_PF_ORGANIC        = 0x14,
        PAT_PF_TOPOGRAPHIC    = 0x15,
        PAT_PF_PLASMA         = 0x16,
        PAT_PF_TENDRILS       = 0x17,
        PAT_PF_LIQUID_GATE    = 0x18,
        PAT_PF_CONCENTRIC_GRID= 0x19,
        PAT_PF_ROW_SEGMENTS   = 0x1A,
        PAT_PF_SHAPES         = 0x1B,
        PAT_PF_DOTS           = 0x1C,
        PAT_PF_WAVE_MATRIX    = 0x1D,
        PAT_PF_RADIAL_PULSE   = 0x1E,
    };

    enum : uint8_t {
        TFM_ROTATION          = 0x00,
        TFM_TRANSLATION       = 0x01,
        TFM_ZOOM              = 0x02,
        TFM_VORTEX            = 0x03,
        TFM_KALEIDOSCOPE      = 0x04,
        TFM_RADIAL_KALEIDO    = 0x05,
        TFM_PALETTE           = 0x06,
        TFM_TILING            = 0x07,
        TFM_FLOW_FIELD        = 0x08,
        TFM_PALETTE_CLIP      = 0x09,
    };

    class WireBuilder {
    public:
        WireBuilder() = default;

        WireBuilder &header(uint8_t paletteId) {
            u8('P'); u8('S'); u8('C'); u8(0);
            u8(0);             // version
            u8(paletteId);
            return *this;
        }

        WireBuilder &u8(uint8_t v) { data_.push_back(v); return *this; }
        WireBuilder &u16(uint16_t v) {
            data_.push_back(static_cast<uint8_t>(v & 0xFF));
            data_.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
            return *this;
        }
        WireBuilder &u32(uint32_t v) {
            data_.push_back(static_cast<uint8_t>(v & 0xFF));
            data_.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
            data_.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
            data_.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
            return *this;
        }
        WireBuilder &i32(int32_t v) {
            uint32_t u;
            std::memcpy(&u, &v, sizeof(u));
            return u32(u);
        }

        // Convenience for the most common signal: constant(permille)
        WireBuilder &sigConstant(uint16_t permille) {
            return u8(SIG_CONSTANT).u16(permille);
        }

        const uint8_t *data() const { return data_.data(); }
        std::size_t size() const { return data_.size(); }

    private:
        std::vector<uint8_t> data_;
    };

    void appendSignal(WireBuilder &w, uint8_t tag) {
        switch (tag) {
            case SIG_CONSTANT:
                w.sigConstant(500);
                return;
            case SIG_C_RANDOM:
                w.u8(SIG_C_RANDOM);
                return;
            case SIG_LINEAR:
            case SIG_QUADRATIC_IN:
            case SIG_QUADRATIC_OUT:
            case SIG_QUADRATIC_IN_OUT:
                w.u8(tag).u32(1000).u8(0);
                return;
            case SIG_SINE:
            case SIG_TRIANGLE:
            case SIG_SQUARE:
            case SIG_SAWTOOTH:
            case SIG_NOISE:
                w.u8(tag).sigConstant(120).i32(0);
                return;
            case SIG_SINE_BOUNDED:
            case SIG_TRIANGLE_BOUNDED:
            case SIG_SQUARE_BOUNDED:
            case SIG_SAWTOOTH_BOUNDED:
            case SIG_NOISE_BOUNDED:
                w.u8(tag).sigConstant(120).sigConstant(200).sigConstant(800);
                return;
            case SIG_SINE_BOUNDED_PH:
            case SIG_TRIANGLE_BOUNDED_PH:
            case SIG_SQUARE_BOUNDED_PH:
            case SIG_SAWTOOTH_BOUNDED_PH:
            case SIG_NOISE_BOUNDED_PH:
                w.u8(tag).sigConstant(120).i32(0).sigConstant(200).sigConstant(800);
                return;
            case SIG_SMAP:
                w.u8(SIG_SMAP).sigConstant(500).sigConstant(100).sigConstant(900);
                return;
            case SIG_SCALE:
                w.u8(SIG_SCALE).sigConstant(500).u16(raw(perMil(500)));
                return;
            default:
                TEST_FAIL_MESSAGE("unknown test signal tag");
        }
    }

    void appendPattern(WireBuilder &w, uint8_t tag) {
        w.u8(tag);
        switch (tag) {
            case PAT_NOISE_BASIC:
                appendSignal(w, SIG_CONSTANT);
                return;
            case PAT_NOISE_FBM:
                w.u8(4);
                return;
            case PAT_NOISE_TURBULENCE:
            case PAT_NOISE_RIDGED:
                return;
            case PAT_TILING:
                w.u16(32).u8(4).u8(static_cast<uint8_t>(TilingPattern::TileShape::HEXAGON));
                return;
            case PAT_REACTION_DIFFUSION:
                w.u8(static_cast<uint8_t>(ReactionDiffusionPattern::Preset::Coral))
                 .u8(20).u8(20).u8(4);
                return;
            case PAT_FLOW_FIELD:
                w.u8(32).u8(3).u8(static_cast<uint8_t>(FlowFieldPattern::EmitterMode::Both));
                for (uint8_t i = 0; i < 8; ++i) appendSignal(w, SIG_CONSTANT);
                return;
            case PAT_TRANSPORT:
                w.u8(16)
                 .u8(static_cast<uint8_t>(TransportPattern::TransportMode::Shockwave))
                 .u8(0);
                for (uint8_t i = 0; i < 4; ++i) appendSignal(w, SIG_CONSTANT);
                return;
            case PAT_SPIRAL:
                w.u8(2).u8(1);
                for (uint8_t i = 0; i < 3; ++i) appendSignal(w, SIG_CONSTANT);
                return;
            case PAT_ANNULI:
                w.u8(8).u8(32).u8(0).u16(80).u16(800);
                return;
            case PAT_FLURRY:
                w.u8(32).u8(1).u8(static_cast<uint8_t>(FlurryPattern::Shape::Line));
                for (uint8_t i = 0; i < 6; ++i) appendSignal(w, SIG_CONSTANT);
                return;
            case PAT_WORLEY:
            case PAT_VORONOI:
                w.i32(256).u8(static_cast<uint8_t>(WorleyAliasing::Fast));
                return;
            case PAT_PF_DUAL_AXIS:
            case PAT_PF_COUNTER_RIBBONS:
            case PAT_PF_QUAD_DIRECTIONAL:
            case PAT_PF_CROSS:
            case PAT_PF_PLASMA:
            case PAT_PF_TENDRILS:
            case PAT_PF_LIQUID_GATE:
                for (uint8_t i = 0; i < 3; ++i) appendSignal(w, SIG_CONSTANT);
                return;
            case PAT_PF_POSTERIZED:
                w.u8(5);
                for (uint8_t i = 0; i < 3; ++i) appendSignal(w, SIG_CONSTANT);
                return;
            case PAT_PF_PETALS:
            case PAT_PF_RIPPLE:
                w.u8(6);
                for (uint8_t i = 0; i < 3; ++i) appendSignal(w, SIG_CONSTANT);
                return;
            case PAT_PF_ORGANIC:
            case PAT_PF_TOPOGRAPHIC:
                w.u8(8).u8(0);
                for (uint8_t i = 0; i < 2; ++i) appendSignal(w, SIG_CONSTANT);
                return;
            case PAT_PF_CONCENTRIC_GRID:
            case PAT_PF_ROW_SEGMENTS:
            case PAT_PF_SHAPES:
            case PAT_PF_DOTS:
            case PAT_PF_WAVE_MATRIX:
            case PAT_PF_RADIAL_PULSE:
                w.u8(6);
                for (uint8_t i = 0; i < 3; ++i) appendSignal(w, SIG_CONSTANT);
                return;
            default:
                TEST_FAIL_MESSAGE("unknown test pattern tag");
        }
    }

    void appendTransform(WireBuilder &w, uint8_t tag) {
        w.u8(tag);
        switch (tag) {
            case TFM_TRANSLATION:
                appendSignal(w, SIG_CONSTANT);
                appendSignal(w, SIG_CONSTANT);
                return;
            case TFM_VORTEX:
                appendSignal(w, SIG_CONSTANT);
                return;
            case TFM_RADIAL_KALEIDO:
                w.u16(6).u8(1);
                return;
            case TFM_TILING:
                w.u8(0).u8(static_cast<uint8_t>(TilingMaths::TileShape::HEXAGON));
                appendSignal(w, SIG_CONSTANT);
                return;
            case TFM_FLOW_FIELD:
                for (uint8_t i = 0; i < 4; ++i) appendSignal(w, SIG_CONSTANT);
                return;
            default:
                TEST_FAIL_MESSAGE("unknown test transform tag");
        }
    }

    void assertDecodeCompileSample(WireBuilder &w) {
        DecodeStatus status;
        auto decoded = decodeScene(w.data(), w.size(), &status);
        TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
        TEST_ASSERT_NOT_NULL(decoded.get());
        decoded->compile();
        decoded->advanceFrame(f16(0xFFFFu), 1000);
        ::CRGB sample = decoded->sample(0, f16(0x4000u), f16(0x4000u));
        (void) sample;
    }

    void assertEveryPrefixRejected(WireBuilder &full) {
        for (std::size_t prefix = 0; prefix < full.size(); ++prefix) {
            DecodeStatus status = DecodeStatus::OK;
            auto scene = decodeScene(full.data(), prefix, &status);
            TEST_ASSERT_NULL(scene.get());
            TEST_ASSERT_NOT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
        }
    }

    // ───── Render-equality helper ─────────────────────────────────────
    //
    // Two scenes are "render-equivalent" if, after compile() and
    // advanceFrame(progress, elapsedMs) at fixed values, sample(...) at a
    // small grid of (angle, radius) points returns the same CRGB bytes.

    void runScene(Scene &s, TimeMillis elapsedMs, ::CRGB out[16]) {
        s.compile();
        // progress = 1.0 (UINT16_MAX) since composer scenes have infinite
        // duration; elapsedMs drives signals.
        s.advanceFrame(f16(0xFFFFu), elapsedMs);
        // 4×4 sample grid in (angle, radius)
        std::size_t i = 0;
        for (uint16_t aIdx = 0; aIdx < 4; ++aIdx) {
            for (uint16_t rIdx = 0; rIdx < 4; ++rIdx) {
                f16 angle  = f16(static_cast<uint16_t>((aIdx * 0xFFFFu) / 3));
                f16 radius = f16(static_cast<uint16_t>((rIdx * 0xFFFFu) / 3));
                out[i++] = s.sample(0, angle, radius);
            }
        }
    }

    bool renderEqual(const ::CRGB a[16], const ::CRGB b[16]) {
        for (std::size_t i = 0; i < 16; ++i) {
            if (a[i].r != b[i].r || a[i].g != b[i].g || a[i].b != b[i].b) return false;
        }
        return true;
    }

    // Build a reference scene programmatically using the same parameters
    // that the corresponding wire-byte test will encode.
    std::unique_ptr<Scene> buildReferenceMinimal() {
        // AnnuliPattern is fully deterministic — no signals, no random
        // initial state. Suitable for byte-equality render comparison.
        const ::CRGBPalette16 *p = paletteById(0);
        LayerBuilder b(annuliPattern(8, 32, false, 80, 800), *p, "ref");
        fl::vector<std::shared_ptr<Layer>> layers;
        layers.push_back(std::make_shared<Layer>(b.build()));
        return std::make_unique<Scene>(std::move(layers));
    }

    std::unique_ptr<Scene> buildReferenceTransportWithStack() {
        const ::CRGBPalette16 *p = paletteById(1);
        LayerBuilder b(transportPattern(
                           16,
                           TransportPattern::TransportMode::Shockwave,
                           constant(300), constant(400), constant(650), constant(500),
                           false),
                       *p, "ref");
        b.addPaletteTransform(PaletteTransform(constant(100)));
        b.addTransform(ZoomTransform(constant(200)));
        b.addTransform(RotationTransform(constant(50), true));
        b.addTransform(KaleidoscopeTransform(4, true));
        fl::vector<std::shared_ptr<Layer>> layers;
        layers.push_back(std::make_shared<Layer>(b.build()));
        return std::make_unique<Scene>(std::move(layers));
    }

    std::unique_ptr<Scene> buildReferenceNestedSmap() {
        // tilingPattern + zoom transform whose scale signal is
        // smap(sine(constant(50), sf16(0)), constant(100), constant(900)).
        // sine with explicit 0 phase offset is deterministic.
        const ::CRGBPalette16 *p = paletteById(2);
        LayerBuilder b(tilingPattern(32, 4, TilingPattern::TileShape::HEXAGON), *p, "ref");
        Sf16Signal nested = smap(
            sine(constant(50), sf16(0)),
            constant(100),
            constant(900)
        );
        b.addTransform(ZoomTransform(std::move(nested)));
        fl::vector<std::shared_ptr<Layer>> layers;
        layers.push_back(std::make_shared<Layer>(b.build()));
        return std::make_unique<Scene>(std::move(layers));
    }

    std::unique_ptr<Scene> buildReferencePfDots() {
        // pfDots with constant signals is deterministic (no random state).
        const ::CRGBPalette16 *p = paletteById(0);
        LayerBuilder b(pfDots(6, constant(500), constant(400), constant(600)),
                       *p, "ref");
        fl::vector<std::shared_ptr<Layer>> layers;
        layers.push_back(std::make_shared<Layer>(b.build()));
        return std::make_unique<Scene>(std::move(layers));
    }

    std::unique_ptr<Scene> buildReferencePaletteChangedPfConcentricGrid() {
        const ::CRGBPalette16 *p = paletteById(1);
        LayerBuilder b(pfConcentricGrid(6, constant(500), constant(500), constant(500)),
                       *p, "ref");
        b.addPaletteTransform(PaletteTransform(sine(constant(120))));
        b.addTransform(ZoomTransform(constant(400)));
        b.addTransform(RotationTransform(constant(60), true));
        fl::vector<std::shared_ptr<Layer>> layers;
        layers.push_back(std::make_shared<Layer>(b.build()));
        return std::make_unique<Scene>(std::move(layers));
    }

    std::unique_ptr<Scene> buildReferencePaletteClip() {
        const ::CRGBPalette16 *p = paletteById(1);
        LayerBuilder b(annuliPattern(8, 32, false, 80, 800), *p, "ref");
        b.addPaletteTransform(PaletteTransform(
            constant(100),
            constant(300),
            perMil(250),
            PipelineContext::PaletteClipPower::Quartic));
        fl::vector<std::shared_ptr<Layer>> layers;
        layers.push_back(std::make_shared<Layer>(b.build()));
        return std::make_unique<Scene>(std::move(layers));
    }

    std::unique_ptr<Scene> buildReferencePaletteColourMask() {
        const ::CRGBPalette16 *p = paletteById(1);
        LayerBuilder b(annuliPattern(8, 32, false, 80, 800), *p, "ref");
        b.addPaletteTransform(PaletteTransform(
            constant(100),
            constant(300),
            perMil(250),
            PipelineContext::PaletteClipPower::Quartic,
            true));  // colourMask
        fl::vector<std::shared_ptr<Layer>> layers;
        layers.push_back(std::make_shared<Layer>(b.build()));
        return std::make_unique<Scene>(std::move(layers));
    }

}  // namespace

// ═════════════════════════════════════════════════════════════════════
// Group 1 — Decode determinism via render-equality
// ═════════════════════════════════════════════════════════════════════

void test_decode_determinism_minimal() {
    // Wire format: minimal scene = AnnuliPattern (deterministic, no signals).
    // ringCount=8, slicesPerRing=32, reverse=false, stepIntervalMs=80, holdMs=800.
    WireBuilder w;
    w.header(0)
     .u8(PAT_ANNULI)
     .u8(8).u8(32).u8(0).u16(80).u16(800)
     .u8(0);  // 0 transforms

    DecodeStatus status;
    auto decoded = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());

    auto reference = buildReferenceMinimal();

    ::CRGB outDecoded[16];
    ::CRGB outReference[16];
    const TimeMillis t = 1500;
    runScene(*decoded, t, outDecoded);
    runScene(*reference, t, outReference);

    TEST_ASSERT_TRUE(renderEqual(outDecoded, outReference));
}

void test_decode_determinism_full_transform_stack() {
    // Wire format: TransportPattern (Shockwave) + 4 transforms.
    WireBuilder w;
    w.header(1)
     .u8(PAT_TRANSPORT)
     .u8(16)                             // gridSize
     .u8(static_cast<uint8_t>(TransportPattern::TransportMode::Shockwave))
     .u8(0)                              // velocityGlow = false
     .sigConstant(300)                   // radial
     .sigConstant(400)                   // angular
     .sigConstant(650)                   // halfLife
     .sigConstant(500)                   // emitter
     .u8(4)                              // 4 transforms
     // 1: PaletteTransform(constant(100))
     .u8(TFM_PALETTE).sigConstant(100)
     // 2: ZoomTransform(constant(200))
     .u8(TFM_ZOOM).sigConstant(200)
     // 3: RotationTransform(constant(50), isAngleTurn=true)
     .u8(TFM_ROTATION).u8(1).sigConstant(50)
     // 4: KaleidoscopeTransform(4, true)
     .u8(TFM_KALEIDOSCOPE).u8(4).u8(1);

    DecodeStatus status;
    auto decoded = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());

    auto reference = buildReferenceTransportWithStack();

    ::CRGB outDecoded[16];
    ::CRGB outReference[16];
    const TimeMillis t = 2500;
    runScene(*decoded, t, outDecoded);
    runScene(*reference, t, outReference);

    TEST_ASSERT_TRUE(renderEqual(outDecoded, outReference));
}

void test_decode_determinism_nested_smap_signal() {
    // Wire format: tilingPattern + ZoomTransform whose scale is
    // smap(sine(constant(50), 0), constant(100), constant(900)).
    WireBuilder w;
    w.header(2)
     .u8(PAT_TILING).u16(32).u8(4).u8(static_cast<uint8_t>(TilingPattern::TileShape::HEXAGON))
     .u8(1)                              // 1 transform
     .u8(TFM_ZOOM)
       .u8(SIG_SMAP)
         .u8(SIG_SINE).sigConstant(50).i32(0)        // sine(constant(50), sf16(0))
         .sigConstant(100)
         .sigConstant(900);

    DecodeStatus status;
    auto decoded = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());

    auto reference = buildReferenceNestedSmap();

    ::CRGB outDecoded[16];
    ::CRGB outReference[16];
    const TimeMillis t = 3000;
    runScene(*decoded, t, outDecoded);
    runScene(*reference, t, outReference);

    TEST_ASSERT_TRUE(renderEqual(outDecoded, outReference));
}

void test_decode_determinism_palette_clip_transform() {
    // Wire format: AnnuliPattern + PaletteTransform(offset, clip, maxFeather, Quartic).
    // Palette clip config bytes (maxFeather, clipPower, colourMask) are encoded
    // before the offset/clip signal slots.
    WireBuilder w;
    w.header(1)
     .u8(PAT_ANNULI)
     .u8(8).u8(32).u8(0).u16(80).u16(800)
     .u8(1)
     .u8(TFM_PALETTE_CLIP)
       .u16(raw(perMil(250)))
       .u8(2)
       .u8(0)
       .sigConstant(100)
       .sigConstant(300);

    DecodeStatus status;
    auto decoded = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());

    auto reference = buildReferencePaletteClip();

    ::CRGB outDecoded[16];
    ::CRGB outReference[16];
    const TimeMillis t = 1500;
    runScene(*decoded, t, outDecoded);
    runScene(*reference, t, outReference);

    TEST_ASSERT_TRUE(renderEqual(outDecoded, outReference));
}

void test_decode_determinism_palette_colour_mask() {
    // Same wire layout as the palette-clip test but with the colourMask byte
    // set to 1, so the decoded transform tints the scene via paletteOffset and
    // uses the pattern value as alpha.
    WireBuilder w;
    w.header(1)
     .u8(PAT_ANNULI)
     .u8(8).u8(32).u8(0).u16(80).u16(800)
     .u8(1)
     .u8(TFM_PALETTE_CLIP)
       .u16(raw(perMil(250)))
       .u8(2)
       .u8(1)
       .sigConstant(100)
       .sigConstant(300);

    DecodeStatus status;
    auto decoded = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());

    auto reference = buildReferencePaletteColourMask();

    ::CRGB outDecoded[16];
    ::CRGB outReference[16];
    const TimeMillis t = 1500;
    runScene(*decoded, t, outDecoded);
    runScene(*reference, t, outReference);

    TEST_ASSERT_TRUE(renderEqual(outDecoded, outReference));
}

void test_decode_determinism_pf_dots() {
    // Wire format: pfDots (PatternFlow cellular field, tag 0x1C).
    // Body: u8 cellCount THEN 3 signals (phaseSpeed, warp, thickness).
    WireBuilder w;
    w.header(0)
     .u8(PAT_PF_DOTS)
     .u8(6)                    // cellCount
     .sigConstant(500)         // phaseSpeed
     .sigConstant(400)         // warp
     .sigConstant(600)         // thickness
     .u8(0);                   // 0 transforms

    DecodeStatus status;
    auto decoded = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());

    auto reference = buildReferencePfDots();

    ::CRGB outDecoded[16];
    ::CRGB outReference[16];
    const TimeMillis t = 2000;
    runScene(*decoded, t, outDecoded);
    runScene(*reference, t, outReference);

    TEST_ASSERT_TRUE(renderEqual(outDecoded, outReference));
}

void test_decode_palette_changed_pf_concentric_grid_repro() {
    // Repro from the WASM composer log: load Concentric Grid, then switch the
    // global palette from Rainbow (0) to Cloud (1). The scene is colour-native
    // and includes palette-offset, zoom, and rotation transforms.
    WireBuilder w;
    w.header(1)
     .u8(PAT_PF_CONCENTRIC_GRID)
     .u8(6)
     .sigConstant(500)
     .sigConstant(500)
     .sigConstant(500)
     .u8(3)
     .u8(TFM_PALETTE)
       .u8(SIG_SINE).sigConstant(120).i32(0)
     .u8(TFM_ZOOM).sigConstant(400)
     .u8(TFM_ROTATION).u8(1).sigConstant(60);

    DecodeStatus status;
    auto decoded = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());

    auto reference = buildReferencePaletteChangedPfConcentricGrid();

    ::CRGB outDecoded[16];
    ::CRGB outReference[16];
    const TimeMillis t = 1500;
    runScene(*decoded, t, outDecoded);
    runScene(*reference, t, outReference);

    TEST_ASSERT_TRUE(renderEqual(outDecoded, outReference));
}

// ═════════════════════════════════════════════════════════════════════
// Group 2 — cRandom / noise existence-only (decode succeeds; non-deterministic)
// ═════════════════════════════════════════════════════════════════════

void test_decode_crandom_succeeds() {
    // NoisePattern with depth = cRandom(). Result is non-deterministic,
    // so we just assert the decoder accepts the scene and produces a
    // non-null Scene. This guards the cRandom path against silent regression.
    WireBuilder w;
    w.header(0).u8(PAT_NOISE_BASIC).u8(SIG_C_RANDOM).u8(0);

    DecodeStatus status;
    auto decoded = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());
}

void test_decode_default_noise_succeeds() {
    // Use noise() with explicit 0 phase to keep it deterministic at the
    // signal level (still pulls in noise sampler though).
    WireBuilder w;
    w.header(0).u8(PAT_NOISE_BASIC)
     .u8(SIG_NOISE).sigConstant(550).i32(0)
     .u8(0);

    DecodeStatus status;
    auto decoded = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());
}

void test_decode_all_signal_tags_compile() {
    static const uint8_t kSignalTags[] = {
        SIG_CONSTANT,
        SIG_C_RANDOM,
        SIG_LINEAR,
        SIG_QUADRATIC_IN,
        SIG_QUADRATIC_OUT,
        SIG_QUADRATIC_IN_OUT,
        SIG_SINE,
        SIG_SINE_BOUNDED,
        SIG_SINE_BOUNDED_PH,
        SIG_TRIANGLE,
        SIG_TRIANGLE_BOUNDED,
        SIG_TRIANGLE_BOUNDED_PH,
        SIG_SQUARE,
        SIG_SQUARE_BOUNDED,
        SIG_SQUARE_BOUNDED_PH,
        SIG_SAWTOOTH,
        SIG_SAWTOOTH_BOUNDED,
        SIG_SAWTOOTH_BOUNDED_PH,
        SIG_NOISE,
        SIG_NOISE_BOUNDED,
        SIG_NOISE_BOUNDED_PH,
        SIG_SMAP,
        SIG_SCALE,
    };

    for (uint8_t tag : kSignalTags) {
        WireBuilder w;
        w.header(0).u8(PAT_NOISE_BASIC);
        appendSignal(w, tag);
        w.u8(0);
        assertDecodeCompileSample(w);
    }
}

void test_decode_all_pattern_tags_compile() {
    static const uint8_t kPatternTags[] = {
        PAT_NOISE_BASIC,
        PAT_NOISE_FBM,
        PAT_NOISE_TURBULENCE,
        PAT_NOISE_RIDGED,
        PAT_TILING,
        PAT_REACTION_DIFFUSION,
        PAT_FLOW_FIELD,
        PAT_TRANSPORT,
        PAT_SPIRAL,
        PAT_ANNULI,
        PAT_FLURRY,
        PAT_WORLEY,
        PAT_VORONOI,
        PAT_PF_DUAL_AXIS,
        PAT_PF_COUNTER_RIBBONS,
        PAT_PF_QUAD_DIRECTIONAL,
        PAT_PF_POSTERIZED,
        PAT_PF_CROSS,
        PAT_PF_PETALS,
        PAT_PF_RIPPLE,
        PAT_PF_ORGANIC,
        PAT_PF_TOPOGRAPHIC,
        PAT_PF_PLASMA,
        PAT_PF_TENDRILS,
        PAT_PF_LIQUID_GATE,
        PAT_PF_CONCENTRIC_GRID,
        PAT_PF_ROW_SEGMENTS,
        PAT_PF_SHAPES,
        PAT_PF_DOTS,
        PAT_PF_WAVE_MATRIX,
        PAT_PF_RADIAL_PULSE,
    };

    for (uint8_t tag : kPatternTags) {
        WireBuilder w;
        w.header(0);
        appendPattern(w, tag);
        w.u8(0);
        assertDecodeCompileSample(w);
    }
}

void test_decode_uncovered_transform_tags_compile() {
    static const uint8_t kTransformTags[] = {
        TFM_TRANSLATION,
        TFM_VORTEX,
        TFM_RADIAL_KALEIDO,
        TFM_TILING,
        TFM_FLOW_FIELD,
    };

    for (uint8_t tag : kTransformTags) {
        WireBuilder w;
        w.header(1);
        appendPattern(w, PAT_ANNULI);
        w.u8(1);
        appendTransform(w, tag);
        assertDecodeCompileSample(w);
    }
}

void test_decode_scene_with_duration_overrides_default() {
    WireBuilder w;
    w.header(0).u8(PAT_NOISE_BASIC).sigConstant(550).u8(0);

    DecodeStatus status;
    auto decoded = decodeSceneWithDuration(w.data(), w.size(), 30000, &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());
    TEST_ASSERT_EQUAL_UINT32(30000, decoded->getDuration());
}

void test_embedded_psc_playlist_provider_decodes_scene() {
    WireBuilder w;
    w.header(0).u8(PAT_NOISE_BASIC).sigConstant(550).u8(0);

    EmbeddedPscScene scenes[] = {
        {"test.psc", w.data(), w.size()},
    };

    EmbeddedPscPlaylistProvider provider(scenes, 1, 30000);
    auto decoded = provider.nextScene();
    TEST_ASSERT_NOT_NULL(decoded.get());
    TEST_ASSERT_EQUAL_UINT32(30000, decoded->getDuration());
}

void test_embedded_psc_playlist_provider_falls_back_after_decode_fail() {
    WireBuilder corrupt;
    corrupt.header(0).u8(0xEE);

    WireBuilder fallbackWire;
    fallbackWire.header(0).u8(PAT_NOISE_BASIC).sigConstant(550).u8(0);

    EmbeddedPscScene scenes[] = {
        {"corrupt.psc", corrupt.data(), corrupt.size()},
    };

    auto fallbackProvider = std::make_unique<DefaultSceneProvider>([&fallbackWire]() {
        return decodeScene(fallbackWire.data(), fallbackWire.size());
    });

    EmbeddedPscPlaylistProvider provider(scenes, 1, 30000, std::move(fallbackProvider));
    auto decoded = provider.nextScene();
    TEST_ASSERT_NOT_NULL(decoded.get());
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, decoded->getDuration());
}

void test_embedded_psc_playlist_provider_has_builtin_fallback() {
    WireBuilder corrupt;
    corrupt.header(0).u8(0xEE);

    EmbeddedPscScene scenes[] = {
        {"corrupt.psc", corrupt.data(), corrupt.size()},
    };

    EmbeddedPscPlaylistProvider provider(scenes, 1, 30000);
    auto decoded = provider.nextScene();
    TEST_ASSERT_NOT_NULL(decoded.get());
    TEST_ASSERT_EQUAL_UINT32(UINT32_MAX, decoded->getDuration());
}

// ═════════════════════════════════════════════════════════════════════
// Group 3 — Cross-implementation golden fixture
// ═════════════════════════════════════════════════════════════════════
//
// Authored as raw byte literals (NOT generated by the C++ encoder
// helpers above). Validates that codec.js / schema.js can produce bytes
// the C++ decoder accepts — i.e. that the JS schema and the C++ tag
// tables agree. If you change a tag value, this test will break first.

void test_decode_golden_fixture() {
    static const uint8_t kGoldenFixture[] = {
        // header
        0x50, 0x53, 0x43, 0x00,   // magic "PSC\0"
        0x00,                      // version 0
        0x00,                      // palette 0 (Rainbow / test palette 0)
        // pattern: NoiseBasic with constant(550)
        0x00,                      // PAT_NOISE_BASIC
        0x00, 0x26, 0x02,          // SIG_CONSTANT, permille = 0x0226 = 550 (LE)
        // 1 transform: Zoom with constant(200)
        0x01,                      // transform count
        0x02,                      // TFM_ZOOM
        0x00, 0xC8, 0x00,          // SIG_CONSTANT, permille = 0x00C8 = 200 (LE)
    };

    DecodeStatus status;
    auto decoded = decodeScene(kGoldenFixture, sizeof(kGoldenFixture), &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());

    // Sanity: scene compiles and renders something.
    decoded->compile();
    decoded->advanceFrame(f16(0xFFFFu), 1000);
    ::CRGB sample = decoded->sample(0, f16(0x4000u), f16(0x4000u));
    (void) sample;  // value depends on noise sampler but call must not crash
}

void test_decode_compile_reported_pf_cross_fixture() {
    // Captured from a browser PSC load that decoded successfully, then trapped
    // in WASM during Scene::compile(). Native must at least decode, compile,
    // advance, and sample it without crashing so sanitizer runs can localize
    // any non-WASM memory issue.
    static const uint8_t kReportedFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x00, 0x00, 0x11, 0x00,
        0xF4, 0x01, 0x00, 0x29, 0x03, 0x00, 0x63, 0x00,
        0x06, 0x07, 0x00, 0x00, 0x11, 0x00, 0xF4, 0x01,
        0x00, 0x89, 0x01, 0x00, 0xE8, 0x03, 0x03, 0x1C,
        0x00, 0xF4, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04,
        0x04, 0x01, 0x02, 0x1D, 0x00, 0xD9, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xAE, 0x00, 0x00, 0x01, 0x1C,
        0x00, 0xB4, 0x01, 0x00, 0x00, 0x00, 0x00, 0x09,
        0x00, 0x80, 0x00, 0x01, 0x00, 0xF4, 0x01, 0x00,
        0x00, 0x00,
    };

    DecodeStatus status;
    auto decoded = decodeScene(kReportedFixture, sizeof(kReportedFixture), &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());

    decoded->compile();
    decoded->advanceFrame(f16(0xFFFFu), 1000);
    ::CRGB sample = decoded->sample(0, f16(0x4000u), f16(0x4000u));
    (void) sample;
}

// ═════════════════════════════════════════════════════════════════════
// Group 4 — Malformed inputs
// ═════════════════════════════════════════════════════════════════════

void test_decode_truncated_at_every_prefix() {
    WireBuilder full;
    full.header(0).u8(PAT_NOISE_BASIC).sigConstant(550).u8(0);

    // Every prefix shorter than the full blob must be rejected without
    // crashing. We don't assert the exact status for every prefix (some
    // prefixes hit BAD_MAGIC, most hit TRUNCATED) — only that decode
    // returns nullptr and a non-OK status.
    for (std::size_t prefix = 0; prefix < full.size(); ++prefix) {
        DecodeStatus status = DecodeStatus::OK;
        auto scene = decodeScene(full.data(), prefix, &status);
        TEST_ASSERT_NULL(scene.get());
        TEST_ASSERT_NOT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    }
}

void test_decode_complex_truncated_at_every_prefix() {
    WireBuilder full;
    full.header(1)
     .u8(PAT_FLOW_FIELD)
     .u8(32)
     .u8(3)
     .u8(static_cast<uint8_t>(FlowFieldPattern::EmitterMode::Both));
    appendSignal(full, SIG_SINE_BOUNDED_PH);
    appendSignal(full, SIG_TRIANGLE_BOUNDED);
    appendSignal(full, SIG_SQUARE);
    appendSignal(full, SIG_SAWTOOTH_BOUNDED_PH);
    appendSignal(full, SIG_NOISE_BOUNDED_PH);
    appendSignal(full, SIG_SMAP);
    appendSignal(full, SIG_SCALE);
    appendSignal(full, SIG_QUADRATIC_OUT);
    full.u8(5);
    appendTransform(full, TFM_TRANSLATION);
    appendTransform(full, TFM_VORTEX);
    appendTransform(full, TFM_RADIAL_KALEIDO);
    appendTransform(full, TFM_TILING);
    appendTransform(full, TFM_FLOW_FIELD);

    assertDecodeCompileSample(full);
    assertEveryPrefixRejected(full);
}

void test_decode_bad_magic() {
    static const uint8_t bytes[] = { 'X', 'X', 'X', 0, 0, 0, 0, 0 };
    DecodeStatus status;
    auto s = decodeScene(bytes, sizeof(bytes), &status);
    TEST_ASSERT_NULL(s.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::BAD_MAGIC), static_cast<int>(status));
}

void test_decode_bad_version() {
    static const uint8_t bytes[] = { 'P', 'S', 'C', 0, /*ver=*/0xFF, 0, 0, 0 };
    DecodeStatus status;
    auto s = decodeScene(bytes, sizeof(bytes), &status);
    TEST_ASSERT_NULL(s.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::BAD_VERSION), static_cast<int>(status));
}

void test_decode_bad_palette_id() {
    WireBuilder w;
    w.header(0xFE)    // unknown palette
     .u8(PAT_NOISE_BASIC).sigConstant(550).u8(0);
    DecodeStatus status;
    auto s = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_NULL(s.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::BAD_ENUM), static_cast<int>(status));
}

void test_decode_unknown_pattern_tag() {
    WireBuilder w;
    w.header(0).u8(0xEE).u8(0);   // tag 0xEE is not a defined pattern
    DecodeStatus status;
    auto s = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_NULL(s.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::UNKNOWN_TAG), static_cast<int>(status));
}

void test_decode_unknown_signal_tag() {
    WireBuilder w;
    w.header(0).u8(PAT_NOISE_BASIC).u8(0xCC).u8(0);   // 0xCC is not a defined signal
    DecodeStatus status;
    auto s = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_NULL(s.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::UNKNOWN_TAG), static_cast<int>(status));
}

void test_decode_unknown_transform_tag() {
    WireBuilder w;
    w.header(0).u8(PAT_NOISE_BASIC).sigConstant(550).u8(1).u8(0xDD);
    DecodeStatus status;
    auto s = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_NULL(s.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::UNKNOWN_TAG), static_cast<int>(status));
}

void test_decode_bad_pattern_enum() {
    // TilingPattern with shape byte > HEXAGON (= 2) → BAD_ENUM.
    WireBuilder w;
    w.header(0).u8(PAT_TILING).u16(32).u8(4).u8(0xFF).u8(0);
    DecodeStatus status;
    auto s = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_NULL(s.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::BAD_ENUM), static_cast<int>(status));
}

void test_decode_bad_palette_clip_power() {
    WireBuilder w;
    w.header(0)
     .u8(PAT_NOISE_BASIC).sigConstant(550)
     .u8(1)
     .u8(TFM_PALETTE_CLIP).u16(raw(perMil(250))).u8(0xFF).u8(0);

    DecodeStatus status;
    auto s = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_NULL(s.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::BAD_ENUM), static_cast<int>(status));
}

// ═════════════════════════════════════════════════════════════════════
// Test harness
// ═════════════════════════════════════════════════════════════════════

#ifdef ARDUINO
void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_decode_determinism_minimal);
    RUN_TEST(test_decode_determinism_full_transform_stack);
    RUN_TEST(test_decode_determinism_nested_smap_signal);
    RUN_TEST(test_decode_determinism_palette_clip_transform);
    RUN_TEST(test_decode_determinism_palette_colour_mask);
    RUN_TEST(test_decode_determinism_pf_dots);
    RUN_TEST(test_decode_palette_changed_pf_concentric_grid_repro);
    RUN_TEST(test_decode_crandom_succeeds);
    RUN_TEST(test_decode_default_noise_succeeds);
    RUN_TEST(test_decode_all_signal_tags_compile);
    RUN_TEST(test_decode_all_pattern_tags_compile);
    RUN_TEST(test_decode_uncovered_transform_tags_compile);
    RUN_TEST(test_decode_scene_with_duration_overrides_default);
    RUN_TEST(test_embedded_psc_playlist_provider_decodes_scene);
    RUN_TEST(test_embedded_psc_playlist_provider_falls_back_after_decode_fail);
    RUN_TEST(test_embedded_psc_playlist_provider_has_builtin_fallback);
    RUN_TEST(test_decode_golden_fixture);
    RUN_TEST(test_decode_compile_reported_pf_cross_fixture);
    RUN_TEST(test_decode_truncated_at_every_prefix);
    RUN_TEST(test_decode_complex_truncated_at_every_prefix);
    RUN_TEST(test_decode_bad_magic);
    RUN_TEST(test_decode_bad_version);
    RUN_TEST(test_decode_bad_palette_id);
    RUN_TEST(test_decode_unknown_pattern_tag);
    RUN_TEST(test_decode_unknown_signal_tag);
    RUN_TEST(test_decode_unknown_transform_tag);
    RUN_TEST(test_decode_bad_pattern_enum);
    RUN_TEST(test_decode_bad_palette_clip_power);
    UNITY_END();
}

void loop() {}
#else
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_decode_determinism_minimal);
    RUN_TEST(test_decode_determinism_full_transform_stack);
    RUN_TEST(test_decode_determinism_nested_smap_signal);
    RUN_TEST(test_decode_determinism_palette_clip_transform);
    RUN_TEST(test_decode_determinism_palette_colour_mask);
    RUN_TEST(test_decode_determinism_pf_dots);
    RUN_TEST(test_decode_palette_changed_pf_concentric_grid_repro);
    RUN_TEST(test_decode_crandom_succeeds);
    RUN_TEST(test_decode_default_noise_succeeds);
    RUN_TEST(test_decode_all_signal_tags_compile);
    RUN_TEST(test_decode_all_pattern_tags_compile);
    RUN_TEST(test_decode_uncovered_transform_tags_compile);
    RUN_TEST(test_decode_scene_with_duration_overrides_default);
    RUN_TEST(test_embedded_psc_playlist_provider_decodes_scene);
    RUN_TEST(test_embedded_psc_playlist_provider_falls_back_after_decode_fail);
    RUN_TEST(test_embedded_psc_playlist_provider_has_builtin_fallback);
    RUN_TEST(test_decode_golden_fixture);
    RUN_TEST(test_decode_compile_reported_pf_cross_fixture);
    RUN_TEST(test_decode_truncated_at_every_prefix);
    RUN_TEST(test_decode_complex_truncated_at_every_prefix);
    RUN_TEST(test_decode_bad_magic);
    RUN_TEST(test_decode_bad_version);
    RUN_TEST(test_decode_bad_palette_id);
    RUN_TEST(test_decode_unknown_pattern_tag);
    RUN_TEST(test_decode_unknown_signal_tag);
    RUN_TEST(test_decode_unknown_transform_tag);
    RUN_TEST(test_decode_bad_pattern_enum);
    RUN_TEST(test_decode_bad_palette_clip_power);
    return UNITY_END();
}
#endif
