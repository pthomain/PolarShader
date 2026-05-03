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
        SIG_QUADRATIC_IN_OUT  = 0x05,
        SIG_SINE              = 0x10,
        SIG_NOISE             = 0x1C,
        SIG_SMAP              = 0x1F,
    };

    enum : uint8_t {
        PAT_NOISE_BASIC       = 0x00,
        PAT_NOISE_FBM         = 0x01,
        PAT_NOISE_TURBULENCE  = 0x02,
        PAT_TILING            = 0x04,
        PAT_TRANSPORT         = 0x07,
        PAT_ANNULI            = 0x09,
    };

    enum : uint8_t {
        TFM_ROTATION          = 0x00,
        TFM_ZOOM              = 0x02,
        TFM_KALEIDOSCOPE      = 0x04,
        TFM_PALETTE           = 0x06,
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

// ═════════════════════════════════════════════════════════════════════
// Test harness
// ═════════════════════════════════════════════════════════════════════

#ifdef ARDUINO
void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_decode_determinism_minimal);
    RUN_TEST(test_decode_determinism_full_transform_stack);
    RUN_TEST(test_decode_determinism_nested_smap_signal);
    RUN_TEST(test_decode_crandom_succeeds);
    RUN_TEST(test_decode_default_noise_succeeds);
    RUN_TEST(test_decode_golden_fixture);
    RUN_TEST(test_decode_truncated_at_every_prefix);
    RUN_TEST(test_decode_bad_magic);
    RUN_TEST(test_decode_bad_version);
    RUN_TEST(test_decode_bad_palette_id);
    RUN_TEST(test_decode_unknown_pattern_tag);
    RUN_TEST(test_decode_unknown_signal_tag);
    RUN_TEST(test_decode_unknown_transform_tag);
    RUN_TEST(test_decode_bad_pattern_enum);
    UNITY_END();
}

void loop() {}
#else
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_decode_determinism_minimal);
    RUN_TEST(test_decode_determinism_full_transform_stack);
    RUN_TEST(test_decode_determinism_nested_smap_signal);
    RUN_TEST(test_decode_crandom_succeeds);
    RUN_TEST(test_decode_default_noise_succeeds);
    RUN_TEST(test_decode_golden_fixture);
    RUN_TEST(test_decode_truncated_at_every_prefix);
    RUN_TEST(test_decode_bad_magic);
    RUN_TEST(test_decode_bad_version);
    RUN_TEST(test_decode_bad_palette_id);
    RUN_TEST(test_decode_unknown_pattern_tag);
    RUN_TEST(test_decode_unknown_signal_tag);
    RUN_TEST(test_decode_unknown_transform_tag);
    RUN_TEST(test_decode_bad_pattern_enum);
    return UNITY_END();
}
#endif
