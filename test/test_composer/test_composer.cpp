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
#include "display/DisplaySpecCodec.h"
#include "display/WebDisplayGeometry.h"
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
        PAT_PALETTE_GLOW      = 0x1F,
        PAT_ROCAILLE          = 0x20,
        PAT_PROTEAN_CLOUDS    = 0x21,
        PAT_XOR               = 0x22,
        PAT_OCTGRAMS          = 0x23,
        PAT_ROTATING_SQUARES  = 0x24,
        PAT_STARRY_PLANES     = 0x25,
        PAT_TRIG_FIELD        = 0x26,
        PAT_STAR_FIELD_TRAVEL = 0x27,
        PAT_RASTER_CONWAY     = 0x2B,
        PAT_RASTER_CYCLIC_CA  = 0x2C,
        PAT_RASTER_BRIANS_BRAIN = 0x2D,
        PAT_RASTER_LIFE_VARIANT = 0x2E,
        PAT_RASTER_ELEMENTARY = 0x2F,
        PAT_RASTER_MATRIX_RAIN = 0x30,
        PAT_RASTER_RIPPLE     = 0x31,
        PAT_RASTER_FOREST_FIRE = 0x32,
        PAT_RASTER_WIREWORLD  = 0x33,
        PAT_RASTER_LANGTON_ANT = 0x34,
        PAT_RASTER_REACTION_DIFF = 0x35,
        PAT_PF_LATTICE        = 0x36,
        PAT_PF_MOIRE          = 0x37,
        PAT_PF_CHLADNI        = 0x38,
        PAT_PF_CHIRP          = 0x39,
        PAT_PF_SPIRAL_ARMS    = 0x3A,
        PAT_PF_RIPPLE_TANK    = 0x3B,
        PAT_NOISE_BASIC_LOOP  = 0x3C,
    };

    enum : uint8_t {
        TFM_ROTATION          = 0x00,
        TFM_TRANSLATION       = 0x01,
        TFM_ZOOM              = 0x02,
        TFM_VORTEX            = 0x03,
        TFM_KALEIDOSCOPE      = 0x04,
        TFM_RADIAL_KALEIDO    = 0x05,
        TFM_TILING            = 0x07,
        TFM_FLOW_FIELD        = 0x08,
        TFM_PALETTE_CLIP      = 0x09,
    };

    constexpr uint8_t LEGACY_TFM_PALETTE = 0x06;

    class WireBuilder {
    public:
        WireBuilder() = default;

        WireBuilder &header(uint8_t paletteId) {
            u8('P'); u8('S'); u8('C'); u8(0);
            u8(1);             // version
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
        WireBuilder &append(const WireBuilder &other) {
            data_.insert(data_.end(), other.data_.begin(), other.data_.end());
            return *this;
        }
        template <typename Fn>
        WireBuilder &record(uint8_t tag, Fn fillBody) {
            WireBuilder body;
            fillBody(body);
            u8(tag).u16(static_cast<uint16_t>(body.size()));
            return append(body);
        }

        // Convenience for the most common signal: constant(permille)
        WireBuilder &sigConstant(uint16_t permille) {
            return record(SIG_CONSTANT, [permille](WireBuilder &body) {
                body.u16(permille);
            });
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
                w.record(SIG_C_RANDOM, [](WireBuilder &) {});
                return;
            case SIG_LINEAR:
            case SIG_QUADRATIC_IN:
            case SIG_QUADRATIC_OUT:
            case SIG_QUADRATIC_IN_OUT:
                w.record(tag, [](WireBuilder &body) {
                    body.u32(1000).u8(0);
                });
                return;
            case SIG_SINE:
            case SIG_TRIANGLE:
            case SIG_SQUARE:
            case SIG_SAWTOOTH:
            case SIG_NOISE:
                w.record(tag, [](WireBuilder &body) {
                    body.sigConstant(120).i32(0);
                });
                return;
            case SIG_SINE_BOUNDED:
            case SIG_TRIANGLE_BOUNDED:
            case SIG_SQUARE_BOUNDED:
            case SIG_SAWTOOTH_BOUNDED:
            case SIG_NOISE_BOUNDED:
                w.record(tag, [](WireBuilder &body) {
                    body.sigConstant(120).sigConstant(200).sigConstant(800);
                });
                return;
            case SIG_SINE_BOUNDED_PH:
            case SIG_TRIANGLE_BOUNDED_PH:
            case SIG_SQUARE_BOUNDED_PH:
            case SIG_SAWTOOTH_BOUNDED_PH:
            case SIG_NOISE_BOUNDED_PH:
                w.record(tag, [](WireBuilder &body) {
                    body.sigConstant(120).i32(0).sigConstant(200).sigConstant(800);
                });
                return;
            case SIG_SMAP:
                w.record(SIG_SMAP, [](WireBuilder &body) {
                    body.sigConstant(500).sigConstant(100).sigConstant(900);
                });
                return;
            case SIG_SCALE:
                w.record(SIG_SCALE, [](WireBuilder &body) {
                    body.sigConstant(500).u16(raw(perMil(500)));
                });
                return;
            default:
                TEST_FAIL_MESSAGE("unknown test signal tag");
        }
    }

    void appendNestedSmapSignal(WireBuilder &w, uint8_t depth) {
        if (depth == 0) {
            w.sigConstant(500);
            return;
        }
        w.record(SIG_SMAP, [depth](WireBuilder &body) {
            appendNestedSmapSignal(body, static_cast<uint8_t>(depth - 1));
            body.sigConstant(0);
            body.sigConstant(1000);
        });
    }

    void appendPattern(WireBuilder &w, uint8_t tag) {
        w.record(tag, [tag](WireBuilder &body) {
            switch (tag) {
                case PAT_NOISE_BASIC:
                    appendSignal(body, SIG_CONSTANT);
                    return;
                case PAT_NOISE_FBM:
                    body.u8(4);
                    return;
                case PAT_NOISE_TURBULENCE:
                case PAT_NOISE_RIDGED:
                    return;
                case PAT_TILING:
                    body.u16(32).u8(4).u8(static_cast<uint8_t>(TilingPattern::TileShape::HEXAGON));
                    return;
                case PAT_REACTION_DIFFUSION:
                    body.u8(static_cast<uint8_t>(ReactionDiffusionPattern::Preset::Coral))
                     .u8(20).u8(20).u8(4);
                    return;
                case PAT_FLOW_FIELD:
                    body.u8(32).u8(3).u8(static_cast<uint8_t>(FlowFieldPattern::EmitterMode::Both));
                    for (uint8_t i = 0; i < 8; ++i) appendSignal(body, SIG_CONSTANT);
                    return;
                case PAT_TRANSPORT:
                    body.u8(16)
                     .u8(static_cast<uint8_t>(TransportPattern::TransportMode::Shockwave))
                     .u8(0);
                    for (uint8_t i = 0; i < 4; ++i) appendSignal(body, SIG_CONSTANT);
                    return;
                case PAT_SPIRAL:
                    body.u8(2).u8(1);
                    for (uint8_t i = 0; i < 3; ++i) appendSignal(body, SIG_CONSTANT);
                    return;
                case PAT_ANNULI:
                    body.u8(8).u8(32).u8(0).u16(80).u16(800);
                    return;
                case PAT_FLURRY:
                    body.u8(32).u8(1).u8(static_cast<uint8_t>(FlurryPattern::Shape::Line));
                    for (uint8_t i = 0; i < 6; ++i) appendSignal(body, SIG_CONSTANT);
                    return;
                case PAT_WORLEY:
                case PAT_VORONOI:
                    body.i32(256).u8(static_cast<uint8_t>(WorleyAliasing::Fast));
                    return;
                case PAT_PF_DUAL_AXIS:
                case PAT_PF_COUNTER_RIBBONS:
                case PAT_PF_QUAD_DIRECTIONAL:
                case PAT_PF_CROSS:
                case PAT_PF_PLASMA:
                case PAT_PF_TENDRILS:
                case PAT_PF_LIQUID_GATE:
                case PAT_PF_LATTICE:
                case PAT_PF_MOIRE:
                    for (uint8_t i = 0; i < 3; ++i) appendSignal(body, SIG_CONSTANT);
                    return;
                case PAT_PF_POSTERIZED:
                    body.u8(5);
                    for (uint8_t i = 0; i < 3; ++i) appendSignal(body, SIG_CONSTANT);
                    return;
                case PAT_PF_PETALS:
                case PAT_PF_RIPPLE:
                case PAT_PF_CHLADNI:
                case PAT_PF_CHIRP:
                case PAT_PF_SPIRAL_ARMS:
                case PAT_PF_RIPPLE_TANK:
                    body.u8(6);
                    for (uint8_t i = 0; i < 3; ++i) appendSignal(body, SIG_CONSTANT);
                    return;
                case PAT_PF_ORGANIC:
                case PAT_PF_TOPOGRAPHIC:
                    body.u8(8).u8(0);
                    for (uint8_t i = 0; i < 2; ++i) appendSignal(body, SIG_CONSTANT);
                    return;
                case PAT_PF_CONCENTRIC_GRID:
                case PAT_PF_ROW_SEGMENTS:
                case PAT_PF_SHAPES:
                case PAT_PF_DOTS:
                case PAT_PF_WAVE_MATRIX:
                case PAT_PF_RADIAL_PULSE:
                    body.u8(6);
                    for (uint8_t i = 0; i < 3; ++i) appendSignal(body, SIG_CONSTANT);
                    return;
                case PAT_PALETTE_GLOW:
                    body.sigConstant(1000);
                    body.sigConstant(500);
                    return;
                case PAT_ROCAILLE:
                    body.sigConstant(333);
                    body.sigConstant(429);
                    body.sigConstant(222);
                    body.sigConstant(500);
                    body.sigConstant(333);
                    body.sigConstant(333);
                    body.sigConstant(727);
                    body.sigConstant(500);
                    body.sigConstant(429);
                    return;
                case PAT_PROTEAN_CLOUDS:
                    body.sigConstant(1000);
                    body.sigConstant(500);
                    body.sigConstant(500);
                    body.sigConstant(500);
                    return;
                case PAT_XOR:
                    body.u8(16).u16(40);
                    return;
                case PAT_OCTGRAMS:
                    body.sigConstant(1000);
                    body.sigConstant(500);
                    body.sigConstant(500);
                    body.sigConstant(500);
                    body.sigConstant(500);
                    return;
                case PAT_ROTATING_SQUARES:
                    body.sigConstant(1000);
                    body.sigConstant(375);
                    body.sigConstant(333);
                    body.sigConstant(500);
                    return;
                case PAT_STARRY_PLANES:
                    body.sigConstant(1000);
                    body.sigConstant(500);
                    body.sigConstant(400);
                    body.sigConstant(500);
                    body.sigConstant(500);
                    return;
                case PAT_TRIG_FIELD:
                    body.sigConstant(379);
                    body.sigConstant(0);
                    body.sigConstant(364);
                    body.sigConstant(500);
                    body.sigConstant(500);
                    body.sigConstant(500);
                    return;
                case PAT_STAR_FIELD_TRAVEL:
                    body.sigConstant(250);
                    body.sigConstant(500);
                    body.sigConstant(500);
                    body.sigConstant(400);
                    body.sigConstant(600);
                    return;
                case PAT_RASTER_CONWAY:
                    body.u16(250).u16(1).u16(350);
                    return;
                case PAT_RASTER_CYCLIC_CA:
                    body.u16(120).u16(1).u8(8).u8(1);
                    return;
                case PAT_RASTER_BRIANS_BRAIN:
                    body.u16(90).u16(1).u16(300);
                    return;
                case PAT_RASTER_LIFE_VARIANT:
                    body.u16(200).u16(1).u16(350).u8(0);
                    return;
                case PAT_RASTER_ELEMENTARY:
                    body.u16(90).u16(1).u8(30);
                    return;
                case PAT_RASTER_MATRIX_RAIN:
                    body.u16(60).u16(1).u8(40);
                    return;
                case PAT_RASTER_RIPPLE:
                    body.u16(40).u16(1).u8(6);
                    return;
                case PAT_RASTER_FOREST_FIRE:
                    body.u16(120).u16(1).u16(40).u16(2);
                    return;
                case PAT_RASTER_WIREWORLD:
                    body.u16(100).u16(1).u16(500);
                    return;
                case PAT_RASTER_LANGTON_ANT:
                    body.u16(30).u16(1).u16(1);
                    return;
                case PAT_RASTER_REACTION_DIFF:
                    body.u8(0).u16(33).u16(1).u16(4);
                    return;
                default:
                    TEST_FAIL_MESSAGE("unknown test pattern tag");
            }
        });
    }

    void appendTransform(WireBuilder &w, uint8_t tag) {
        w.record(tag, [tag](WireBuilder &body) {
            switch (tag) {
                case TFM_TRANSLATION:
                    appendSignal(body, SIG_CONSTANT);
                    appendSignal(body, SIG_CONSTANT);
                    return;
                case TFM_VORTEX:
                    appendSignal(body, SIG_CONSTANT);
                    return;
                case TFM_RADIAL_KALEIDO:
                    body.u16(6).u8(1);
                    return;
                case TFM_TILING:
                    body.u8(0).u8(static_cast<uint8_t>(TilingMaths::TileShape::HEXAGON));
                    appendSignal(body, SIG_CONSTANT);
                    return;
                case TFM_FLOW_FIELD:
                    for (uint8_t i = 0; i < 4; ++i) appendSignal(body, SIG_CONSTANT);
                    return;
                default:
                    TEST_FAIL_MESSAGE("unknown test transform tag");
            }
        });
    }

    void assertDecodeCompileSample(WireBuilder &w) {
        DecodeStatus status;
        auto decoded = decodeScene(w.data(), w.size(), &status);
        TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
        TEST_ASSERT_NOT_NULL(decoded.get());
        decoded->compile();
        decoded->advanceFrame(u0x16(0xFFFFu), 1000);
        ::CRGB sample = decoded->sample(0, u0x16(0x4000u), u0x16(0x4000u));
        (void) sample;
    }

    void assertDecodeCompileSampleBytes(
        const char *fixtureName,
        const uint8_t *data,
        std::size_t size,
        bool rasterDisplay = false
    ) {
        DecodeStatus status = DecodeStatus::OK;
        auto decoded = decodeScene(data, size, &status);
        TEST_ASSERT_EQUAL_MESSAGE(static_cast<int>(DecodeStatus::OK), static_cast<int>(status), fixtureName);
        TEST_ASSERT_NOT_NULL_MESSAGE(decoded.get(), fixtureName);
        if (rasterDisplay) {
            decoded->compile(RasterDisplayInfo{true, 4, 4, 16});
            const RenderPoint point{
                u0x16(0x4000u),
                u0x16(0x4000u),
                RasterPoint{true, 1, 1, 4, 4}
            };
            ::CRGB sample = decoded->sample(0, point);
            (void) sample;
        } else {
            decoded->compile();
            decoded->advanceFrame(u0x16(0xFFFFu), 1000);
            ::CRGB sample = decoded->sample(0, u0x16(0x4000u), u0x16(0x4000u));
            (void) sample;
        }
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
        s.advanceFrame(u0x16(0xFFFFu), elapsedMs);
        // 4×4 sample grid in (angle, radius)
        std::size_t i = 0;
        for (uint16_t aIdx = 0; aIdx < 4; ++aIdx) {
            for (uint16_t rIdx = 0; rIdx < 4; ++rIdx) {
                u0x16 angle  = u0x16(static_cast<uint16_t>((aIdx * 0xFFFFu) / 3));
                u0x16 radius = u0x16(static_cast<uint16_t>((rIdx * 0xFFFFu) / 3));
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
        b.addPaletteTransform(PaletteTransform(
            constant(100),
            constant(0),
            u0x16(32768),
            PipelineContext::PaletteTintMode::HueRemap));
        b.addTransform(ZoomTransform(constant(200)));
        b.addTransform(RotationTransform(constant(50), true));
        b.addTransform(KaleidoscopeTransform(4, true));
        fl::vector<std::shared_ptr<Layer>> layers;
        layers.push_back(std::make_shared<Layer>(b.build()));
        return std::make_unique<Scene>(std::move(layers));
    }

    std::unique_ptr<Scene> buildReferenceNestedSmap() {
        // tilingPattern + zoom transform whose scale signal is
        // smap(sine(constant(50), s0x16(0)), constant(100), constant(900)).
        // sine with explicit 0 phase offset is deterministic.
        const ::CRGBPalette16 *p = paletteById(2);
        LayerBuilder b(tilingPattern(32, 4, TilingPattern::TileShape::HEXAGON), *p, "ref");
        S0x16Signal nested = smap(
            sine(constant(50), s0x16(0)),
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
        b.addPaletteTransform(PaletteTransform(
            sine(constant(120)),
            constant(0),
            u0x16(32768),
            PipelineContext::PaletteTintMode::HueRemap));
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
            perMil(250)));
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
            PipelineContext::PaletteTintMode::ColourMask));
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
    w.header(0);
    w.record(PAT_ANNULI, [](WireBuilder &body) {
        body.u8(8).u8(32).u8(0).u16(80).u16(800);
    });
    w.u8(0);  // 0 transforms

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
    w.header(1);
    w.record(PAT_TRANSPORT, [](WireBuilder &body) {
        body.u8(16)                             // gridSize
            .u8(static_cast<uint8_t>(TransportPattern::TransportMode::Shockwave))
            .u8(0)                              // velocityGlow = false
            .sigConstant(300)                   // radial
            .sigConstant(400)                   // angular
            .sigConstant(650)                   // halfLife
            .sigConstant(500);                  // emitter
    });
    w.u8(4);                                    // 4 transforms
    w.record(TFM_PALETTE_CLIP, [](WireBuilder &body) {
        body.u16(32768).u8(0).sigConstant(100).sigConstant(0);
    });
    w.record(TFM_ZOOM, [](WireBuilder &body) {
        body.sigConstant(200);
    });
    w.record(TFM_ROTATION, [](WireBuilder &body) {
        body.u8(1).sigConstant(50);
    });
    w.record(TFM_KALEIDOSCOPE, [](WireBuilder &body) {
        body.u8(4).u8(1);
    });

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
    w.header(2);
    w.record(PAT_TILING, [](WireBuilder &body) {
        body.u16(32).u8(4).u8(static_cast<uint8_t>(TilingPattern::TileShape::HEXAGON));
    });
    w.u8(1);                              // 1 transform
    w.record(TFM_ZOOM, [](WireBuilder &body) {
        body.record(SIG_SMAP, [](WireBuilder &signalBody) {
            signalBody.record(SIG_SINE, [](WireBuilder &sineBody) {
                sineBody.sigConstant(50).i32(0);
            });
            signalBody.sigConstant(100).sigConstant(900);
        });
    });

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
    // Wire format: AnnuliPattern + PaletteTransform(offset, clip, maxFeather).
    // Palette clip config bytes (maxFeather, tintMode) are encoded before the
    // offset/clip signal slots.
    WireBuilder w;
    w.header(1);
    w.record(PAT_ANNULI, [](WireBuilder &body) {
        body.u8(8).u8(32).u8(0).u16(80).u16(800);
    });
    w.u8(1);
    w.record(TFM_PALETTE_CLIP, [](WireBuilder &body) {
        body.u16(raw(perMil(250))).u8(0).sigConstant(100).sigConstant(300);
    });

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
    // Same wire layout as the palette-clip test but with the tintMode byte set
    // to 1 (colour-mask), so the decoded transform tints the scene via
    // paletteOffset and uses the pattern value as alpha.
    WireBuilder w;
    w.header(1);
    w.record(PAT_ANNULI, [](WireBuilder &body) {
        body.u8(8).u8(32).u8(0).u16(80).u16(800);
    });
    w.u8(1);
    w.record(TFM_PALETTE_CLIP, [](WireBuilder &body) {
        body.u16(raw(perMil(250))).u8(1).sigConstant(100).sigConstant(300);
    });

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
    w.header(0);
    w.record(PAT_PF_DOTS, [](WireBuilder &body) {
        body.u8(6)                    // cellCount
            .sigConstant(500)         // phaseSpeed
            .sigConstant(400)         // warp
            .sigConstant(600);        // thickness
    });
    w.u8(0);                   // 0 transforms

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
    w.header(1);
    w.record(PAT_PF_CONCENTRIC_GRID, [](WireBuilder &body) {
        body.u8(6).sigConstant(500).sigConstant(500).sigConstant(500);
    });
    w.u8(3);
    w.record(TFM_PALETTE_CLIP, [](WireBuilder &body) {
        body.u16(32768).u8(0);
        body.record(SIG_SINE, [](WireBuilder &signalBody) {
            signalBody.sigConstant(120).i32(0);
        });
        body.sigConstant(0);
    });
    w.record(TFM_ZOOM, [](WireBuilder &body) {
        body.sigConstant(400);
    });
    w.record(TFM_ROTATION, [](WireBuilder &body) {
        body.u8(1).sigConstant(60);
    });

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
    w.header(0);
    w.record(PAT_NOISE_BASIC, [](WireBuilder &body) {
        body.record(SIG_C_RANDOM, [](WireBuilder &) {});
    });
    w.u8(0);

    DecodeStatus status;
    auto decoded = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());
}

void test_decode_default_noise_succeeds() {
    // Explicit 0 phase is accepted and resolved to a random noise phase.
    WireBuilder w;
    w.header(0);
    w.record(PAT_NOISE_BASIC, [](WireBuilder &body) {
        body.record(SIG_NOISE, [](WireBuilder &signalBody) {
            signalBody.sigConstant(550).i32(0);
        });
    });
    w.u8(0);

    DecodeStatus status;
    auto decoded = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());
}

void test_decode_noise_basic_loop_golden() {
    // PAT_NOISE_BASIC_LOOP: u16 loopPeriodMs (10000) then depthSpeed signal.
    static const uint8_t kFixture[] = {
        0x50, 0x53, 0x43, 0x00,   // magic "PSC\0"
        0x01,                      // version 1
        0x00,                      // palette 0
        0x3C, 0x07, 0x00,          // PAT_NOISE_BASIC_LOOP, body length 7
        0x10, 0x27,                // loopPeriodMs = 10000 (LE)
        0x00, 0x02, 0x00, 0x26, 0x02, // SIG_CONSTANT, len 2, 550
        0x00,                      // transform count
    };

    DecodeStatus status = DecodeStatus::OK;
    auto decoded = decodeScene(kFixture, sizeof(kFixture), &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());
    decoded->compile();
    decoded->advanceFrame(u0x16(0x8000u), 5000);
    ::CRGB sample = decoded->sample(0, u0x16(0x4000u), u0x16(0x4000u));
    (void) sample;
}

void test_decode_noise_basic_loop_rejects_zero_period() {
    static const uint8_t kFixture[] = {
        0x50, 0x53, 0x43, 0x00,   // magic "PSC\0"
        0x01,                      // version 1
        0x00,                      // palette 0
        0x3C, 0x07, 0x00,          // PAT_NOISE_BASIC_LOOP, body length 7
        0x00, 0x00,                // loopPeriodMs = 0 (rejected)
        0x00, 0x02, 0x00, 0x26, 0x02, // SIG_CONSTANT, len 2, 550
        0x00,                      // transform count
    };

    DecodeStatus status = DecodeStatus::OK;
    auto decoded = decodeScene(kFixture, sizeof(kFixture), &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::BAD_VALUE), static_cast<int>(status));
    TEST_ASSERT_NULL(decoded.get());
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
        w.header(0);
        w.record(PAT_NOISE_BASIC, [tag](WireBuilder &body) {
            appendSignal(body, tag);
        });
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
        PAT_PALETTE_GLOW,
        PAT_ROCAILLE,
        PAT_PROTEAN_CLOUDS,
        PAT_XOR,
        PAT_OCTGRAMS,
        PAT_ROTATING_SQUARES,
        PAT_STARRY_PLANES,
        PAT_TRIG_FIELD,
        PAT_STAR_FIELD_TRAVEL,
        PAT_PF_LATTICE,
        PAT_PF_MOIRE,
        PAT_PF_CHLADNI,
        PAT_PF_CHIRP,
        PAT_PF_SPIRAL_ARMS,
        PAT_PF_RIPPLE_TANK,
        PAT_RASTER_CONWAY,
        PAT_RASTER_CYCLIC_CA,
        PAT_RASTER_BRIANS_BRAIN,
        PAT_RASTER_LIFE_VARIANT,
        PAT_RASTER_ELEMENTARY,
        PAT_RASTER_MATRIX_RAIN,
        PAT_RASTER_RIPPLE,
        PAT_RASTER_FOREST_FIRE,
        PAT_RASTER_WIREWORLD,
        PAT_RASTER_LANGTON_ANT,
        PAT_RASTER_REACTION_DIFF,
    };

    for (uint8_t tag : kPatternTags) {
        WireBuilder w;
        w.header(0);
        appendPattern(w, tag);
        w.u8(0);
        assertDecodeCompileSample(w);
    }
}

void test_decode_legacy_palette_glow_without_speed_signal() {
    WireBuilder w;
    w.header(0);
    w.record(PAT_PALETTE_GLOW, [](WireBuilder &) {});
    w.u8(0);
    assertDecodeCompileSample(w);
}

void test_decode_palette_glow_without_tile_scale_signal() {
    WireBuilder w;
    w.header(0);
    w.record(PAT_PALETTE_GLOW, [](WireBuilder &body) {
        body.sigConstant(1000);
    });
    w.u8(0);
    assertDecodeCompileSample(w);
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

void test_decode_raster_conway_allows_palette_transform() {
    WireBuilder w;
    w.header(0);
    appendPattern(w, PAT_RASTER_CONWAY);
    w.u8(1);
    w.record(TFM_PALETTE_CLIP, [](WireBuilder &body) {
        body.u16(32768).u8(0);
        appendSignal(body, SIG_CONSTANT);
        body.sigConstant(0);
    });
    assertDecodeCompileSample(w);
}

void test_decode_raster_conway_compiles_with_raster_display() {
    WireBuilder w;
    w.header(0);
    w.record(PAT_RASTER_CONWAY, [](WireBuilder &body) {
        body.u16(250).u16(1).u16(1000);
    });
    w.u8(0);

    DecodeStatus status = DecodeStatus::OK;
    auto decoded = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());

    decoded->compile(RasterDisplayInfo{true, 3, 3, 9});
    const RenderPoint center{
        u0x16(0),
        u0x16(0),
        RasterPoint{true, 1, 1, 3, 3}
    };
    const ::CRGB sample = decoded->sample(0, center);
    TEST_ASSERT_TRUE(sample.r != 0 || sample.g != 0 || sample.b != 0);
}

void test_decode_raster_conway_rejects_uv_transform() {
    WireBuilder w;
    w.header(0);
    appendPattern(w, PAT_RASTER_CONWAY);
    w.u8(1);
    appendTransform(w, TFM_TILING);

    DecodeStatus status = DecodeStatus::OK;
    auto decoded = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_NULL(decoded.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::BAD_ENUM), static_cast<int>(status));
}

void test_decode_scene_with_duration_overrides_default() {
    WireBuilder w;
    w.header(0);
    w.record(PAT_NOISE_BASIC, [](WireBuilder &body) {
        body.sigConstant(550);
    });
    w.u8(0);

    DecodeStatus status;
    auto decoded = decodeSceneWithDuration(w.data(), w.size(), 30000, &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());
    TEST_ASSERT_EQUAL_UINT32(30000, decoded->getDuration());
}

void test_embedded_psc_playlist_provider_decodes_scene() {
    WireBuilder w;
    w.header(0);
    w.record(PAT_NOISE_BASIC, [](WireBuilder &body) {
        body.sigConstant(550);
    });
    w.u8(0);

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
    corrupt.header(0).record(0xEE, [](WireBuilder &) {});

    WireBuilder fallbackWire;
    fallbackWire.header(0);
    fallbackWire.record(PAT_NOISE_BASIC, [](WireBuilder &body) {
        body.sigConstant(550);
    });
    fallbackWire.u8(0);

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
    corrupt.header(0).record(0xEE, [](WireBuilder &) {});

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
        0x01,                      // version 1
        0x00,                      // palette 0 (Rainbow / test palette 0)
        // pattern: NoiseBasic with constant(550)
        0x00, 0x05, 0x00,          // PAT_NOISE_BASIC, body length 5
        0x00, 0x02, 0x00, 0x26, 0x02, // SIG_CONSTANT, body length 2, 550
        // 1 transform: Zoom with constant(200)
        0x01,                      // transform count
        0x02, 0x05, 0x00,          // TFM_ZOOM, body length 5
        0x00, 0x02, 0x00, 0xC8, 0x00, // SIG_CONSTANT, body length 2, 200
    };

    DecodeStatus status;
    auto decoded = decodeScene(kGoldenFixture, sizeof(kGoldenFixture), &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());

    // Sanity: scene compiles and renders something.
    decoded->compile();
    decoded->advanceFrame(u0x16(0xFFFFu), 1000);
    ::CRGB sample = decoded->sample(0, u0x16(0x4000u), u0x16(0x4000u));
    (void) sample;  // value depends on noise sampler but call must not crash
}

void test_decode_v1_length_prefixed_fixture() {
    static const uint8_t kV1Fixture[] = {
        0x50, 0x53, 0x43, 0x00,
        0x01,                     // version 1
        0x00,                     // palette 0
        0x00, 0x05, 0x00,         // PAT_NOISE_BASIC, body length 5
        0x00, 0x02, 0x00, 0x26, 0x02, // SIG_CONSTANT, body length 2, 550
        0x01,                     // transform count
        0x02, 0x05, 0x00,         // TFM_ZOOM, body length 5
        0x00, 0x02, 0x00, 0xC8, 0x00, // SIG_CONSTANT, body length 2, 200
    };

    DecodeStatus status;
    auto decoded = decodeScene(kV1Fixture, sizeof(kV1Fixture), &status);
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
    TEST_ASSERT_NOT_NULL(decoded.get());
    decoded->compile();
}

void test_decode_js_generated_lockstep_fixtures() {
    // These byte arrays were generated by web/sketches/composer/codec.js from
    // the web schema. They intentionally do not use WireBuilder, so they catch
    // JS/C++ tag or body-layout drift for newer codec surface area.
    static const uint8_t kJsXorFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x22, 0x03, 0x00, 0x10, 0x28, 0x00,
        0x00,
    };
    static const uint8_t kJsConwayPaletteFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x2b, 0x06, 0x00, 0xfa, 0x00, 0x01,
        0x00, 0x5e, 0x01, 0x01, 0x09, 0x0d, 0x00, 0x00, 0x80, 0x02, 0x00, 0x02,
        0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00,
    };
    static const uint8_t kJsNestedSignalsPaletteClipFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x02, 0x00, 0x19, 0x00, 0x1f, 0x16, 0x00,
        0x10, 0x09, 0x00, 0x00, 0x02, 0x00, 0x32, 0x00, 0xd2, 0x04, 0x00, 0x00,
        0x00, 0x02, 0x00, 0x64, 0x00, 0x00, 0x02, 0x00, 0x84, 0x03, 0x02, 0x09,
        0x21, 0x00, 0x00, 0x40, 0x01, 0x10, 0x09, 0x00, 0x00, 0x02, 0x00, 0x78,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x1d, 0x0f, 0x00, 0x00, 0x02, 0x00, 0xd9,
        0x00, 0x00, 0x02, 0x00, 0x64, 0x00, 0x00, 0x02, 0x00, 0x20, 0x03, 0x02,
        0x1b, 0x00, 0x20, 0x18, 0x00, 0x1b, 0x13, 0x00, 0x00, 0x02, 0x00, 0x4d,
        0x01, 0x4d, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x96, 0x00, 0x00, 0x02,
        0x00, 0x52, 0x03, 0x00, 0x80,
    };
    static const uint8_t kJsPfDualAxisFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x0d, 0x0f, 0x00, 0x00, 0x02, 0x00,
        0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01,
        0x00,
    };
    static const uint8_t kJsPfCounterRibbonsFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x0e, 0x0f, 0x00, 0x00, 0x02, 0x00,
        0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01,
        0x00,
    };
    static const uint8_t kJsPfQuadDirectionalFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x0f, 0x0f, 0x00, 0x00, 0x02, 0x00,
        0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01,
        0x00,
    };
    static const uint8_t kJsPfPosterizedFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x10, 0x10, 0x00, 0x05, 0x00, 0x02,
        0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4,
        0x01, 0x00,
    };
    static const uint8_t kJsPfCrossFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x11, 0x0f, 0x00, 0x00, 0x02, 0x00,
        0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01,
        0x00,
    };
    static const uint8_t kJsPfPetalsFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x12, 0x10, 0x00, 0x06, 0x00, 0x02,
        0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4,
        0x01, 0x00,
    };
    static const uint8_t kJsPfRippleFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x13, 0x10, 0x00, 0x06, 0x00, 0x02,
        0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4,
        0x01, 0x00,
    };
    static const uint8_t kJsPfOrganicFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x14, 0x0c, 0x00, 0x0a, 0x00, 0x00,
        0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01, 0x00,
    };
    static const uint8_t kJsPfTopographicFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x15, 0x0c, 0x00, 0x08, 0x00, 0x00,
        0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01, 0x00,
    };
    static const uint8_t kJsPfPlasmaFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x16, 0x0f, 0x00, 0x00, 0x02, 0x00,
        0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01,
        0x00,
    };
    static const uint8_t kJsPfTendrilsFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x17, 0x0f, 0x00, 0x00, 0x02, 0x00,
        0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01,
        0x00,
    };
    static const uint8_t kJsPfLiquidGateFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x18, 0x0f, 0x00, 0x00, 0x02, 0x00,
        0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01,
        0x00,
    };
    static const uint8_t kJsPfConcentricGridFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x19, 0x10, 0x00, 0x06, 0x00, 0x02,
        0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4,
        0x01, 0x00,
    };
    static const uint8_t kJsPfRowSegmentsFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x1a, 0x10, 0x00, 0x06, 0x00, 0x02,
        0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4,
        0x01, 0x00,
    };
    static const uint8_t kJsPfShapesFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x1b, 0x10, 0x00, 0x06, 0x00, 0x02,
        0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4,
        0x01, 0x00,
    };
    static const uint8_t kJsPfDotsFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x1c, 0x10, 0x00, 0x06, 0x00, 0x02,
        0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4,
        0x01, 0x00,
    };
    static const uint8_t kJsPfWaveMatrixFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x1d, 0x10, 0x00, 0x06, 0x00, 0x02,
        0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4,
        0x01, 0x00,
    };
    static const uint8_t kJsPfRadialPulseFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x00, 0x1e, 0x10, 0x00, 0x06, 0x00, 0x02,
        0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0xf4,
        0x01, 0x00,
    };
    static const uint8_t kJsRasterCyclicCaFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x01, 0x2c, 0x06, 0x00, 0x78, 0x00, 0x00,
        0x00, 0x08, 0x01, 0x01, 0x09, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
        0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00,
    };
    static const uint8_t kJsRasterBriansBrainFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x01, 0x2d, 0x06, 0x00, 0x5a, 0x00, 0x00,
        0x00, 0x2c, 0x01, 0x01, 0x09, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
        0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00,
    };
    static const uint8_t kJsRasterLifeVariantFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x01, 0x2e, 0x07, 0x00, 0xc8, 0x00, 0x00,
        0x00, 0x5e, 0x01, 0x00, 0x01, 0x09, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x02, 0x00, 0xf4, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00,
    };
    static const uint8_t kJsRasterElementaryFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x01, 0x2f, 0x05, 0x00, 0x5a, 0x00, 0x00,
        0x00, 0x1e, 0x01, 0x09, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
        0xf4, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00,
    };
    static const uint8_t kJsRasterMatrixRainFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x01, 0x30, 0x05, 0x00, 0x3c, 0x00, 0x00,
        0x00, 0x28, 0x01, 0x09, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
        0xf4, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00,
    };
    static const uint8_t kJsRasterRippleFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x01, 0x01, 0x31, 0x05, 0x00, 0x28, 0x00, 0x00,
        0x00, 0x06, 0x01, 0x09, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00,
        0xf4, 0x01, 0x00, 0x02, 0x00, 0x00, 0x00,
    };

    struct Fixture {
        const char *name;
        const uint8_t *data;
        std::size_t size;
        bool raster;
    };

    static const Fixture kFixtures[] = {
        {"js xor", kJsXorFixture, sizeof(kJsXorFixture), false},
        {"js conway paletteClip", kJsConwayPaletteFixture, sizeof(kJsConwayPaletteFixture), true},
        {"js nested signals paletteClip", kJsNestedSignalsPaletteClipFixture, sizeof(kJsNestedSignalsPaletteClipFixture), false},
        {"js pfDualAxis", kJsPfDualAxisFixture, sizeof(kJsPfDualAxisFixture), false},
        {"js pfCounterRibbons", kJsPfCounterRibbonsFixture, sizeof(kJsPfCounterRibbonsFixture), false},
        {"js pfQuadDirectional", kJsPfQuadDirectionalFixture, sizeof(kJsPfQuadDirectionalFixture), false},
        {"js pfPosterized", kJsPfPosterizedFixture, sizeof(kJsPfPosterizedFixture), false},
        {"js pfCross", kJsPfCrossFixture, sizeof(kJsPfCrossFixture), false},
        {"js pfPetals", kJsPfPetalsFixture, sizeof(kJsPfPetalsFixture), false},
        {"js pfRipple", kJsPfRippleFixture, sizeof(kJsPfRippleFixture), false},
        {"js pfOrganic", kJsPfOrganicFixture, sizeof(kJsPfOrganicFixture), false},
        {"js pfTopographic", kJsPfTopographicFixture, sizeof(kJsPfTopographicFixture), false},
        {"js pfPlasma", kJsPfPlasmaFixture, sizeof(kJsPfPlasmaFixture), false},
        {"js pfTendrils", kJsPfTendrilsFixture, sizeof(kJsPfTendrilsFixture), false},
        {"js pfLiquidGate", kJsPfLiquidGateFixture, sizeof(kJsPfLiquidGateFixture), false},
        {"js pfConcentricGrid", kJsPfConcentricGridFixture, sizeof(kJsPfConcentricGridFixture), false},
        {"js pfRowSegments", kJsPfRowSegmentsFixture, sizeof(kJsPfRowSegmentsFixture), false},
        {"js pfShapes", kJsPfShapesFixture, sizeof(kJsPfShapesFixture), false},
        {"js pfDots", kJsPfDotsFixture, sizeof(kJsPfDotsFixture), false},
        {"js pfWaveMatrix", kJsPfWaveMatrixFixture, sizeof(kJsPfWaveMatrixFixture), false},
        {"js pfRadialPulse", kJsPfRadialPulseFixture, sizeof(kJsPfRadialPulseFixture), false},
        {"js rasterCyclicCA paletteClip", kJsRasterCyclicCaFixture, sizeof(kJsRasterCyclicCaFixture), true},
        {"js rasterBriansBrain paletteClip", kJsRasterBriansBrainFixture, sizeof(kJsRasterBriansBrainFixture), true},
        {"js rasterLifeVariant paletteClip", kJsRasterLifeVariantFixture, sizeof(kJsRasterLifeVariantFixture), true},
        {"js rasterElementary paletteClip", kJsRasterElementaryFixture, sizeof(kJsRasterElementaryFixture), true},
        {"js rasterMatrixRain paletteClip", kJsRasterMatrixRainFixture, sizeof(kJsRasterMatrixRainFixture), true},
        {"js rasterRipple paletteClip", kJsRasterRippleFixture, sizeof(kJsRasterRippleFixture), true},
    };

    for (const Fixture &fixture : kFixtures) {
        assertDecodeCompileSampleBytes(fixture.name, fixture.data, fixture.size, fixture.raster);
    }
}

void test_decode_v1_rejects_trailing_pattern_body_bytes() {
    static const uint8_t kBadV1Fixture[] = {
        0x50, 0x53, 0x43, 0x00,
        0x01,
        0x00,
        0x00, 0x06, 0x00,         // one extra byte in pattern body
        0x00, 0x02, 0x00, 0x26, 0x02, 0x00,
        0x00,
    };

    DecodeStatus status = DecodeStatus::OK;
    auto decoded = decodeScene(kBadV1Fixture, sizeof(kBadV1Fixture), &status);
    TEST_ASSERT_NULL(decoded.get());
    TEST_ASSERT_NOT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
}

void test_decode_v1_rejects_trailing_signal_body_bytes() {
    static const uint8_t kBadV1Fixture[] = {
        0x50, 0x53, 0x43, 0x00,
        0x01,
        0x00,
        0x00, 0x06, 0x00,         // NoiseBasic pattern body length 6
        0x00, 0x03, 0x00, 0x26, 0x02, 0x00, // constant signal with one extra body byte
        0x00,
    };

    DecodeStatus status = DecodeStatus::OK;
    auto decoded = decodeScene(kBadV1Fixture, sizeof(kBadV1Fixture), &status);
    TEST_ASSERT_NULL(decoded.get());
    TEST_ASSERT_NOT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
}

void test_decode_v1_rejects_trailing_transform_body_bytes() {
    static const uint8_t kBadV1Fixture[] = {
        0x50, 0x53, 0x43, 0x00,
        0x01,
        0x00,
        0x00, 0x05, 0x00,
        0x00, 0x02, 0x00, 0x26, 0x02,
        0x01,
        0x02, 0x06, 0x00,         // Zoom transform body length 6
        0x00, 0x02, 0x00, 0xc8, 0x00, 0x00, // constant signal + one extra transform byte
    };

    DecodeStatus status = DecodeStatus::OK;
    auto decoded = decodeScene(kBadV1Fixture, sizeof(kBadV1Fixture), &status);
    TEST_ASSERT_NULL(decoded.get());
    TEST_ASSERT_NOT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
}

void test_decode_v1_rejects_trailing_scene_bytes() {
    static const uint8_t kBadV1Fixture[] = {
        0x50, 0x53, 0x43, 0x00,
        0x01,
        0x00,
        0x00, 0x05, 0x00,
        0x00, 0x02, 0x00, 0x26, 0x02,
        0x00,
        0x00,
    };

    DecodeStatus status = DecodeStatus::OK;
    auto decoded = decodeScene(kBadV1Fixture, sizeof(kBadV1Fixture), &status);
    TEST_ASSERT_NULL(decoded.get());
    TEST_ASSERT_NOT_EQUAL(static_cast<int>(DecodeStatus::OK), static_cast<int>(status));
}

void test_decode_rejects_legacy_reported_fixture() {
    // Captured before PSC v1 length-prefixing. Legacy v0 PSC is intentionally
    // rejected at runtime after the playlist has been migrated.
    static const uint8_t kReportedFixture[] = {
        0x50, 0x53, 0x43, 0x00, 0x00, 0x00, 0x11, 0x00,
        0xF4, 0x01, 0x00, 0x29, 0x03, 0x00, 0x63, 0x00,
        0x06, 0x07, 0x00, 0x00, 0x11, 0x00, 0xF4, 0x01,
        0x00, 0x89, 0x01, 0x00, 0xE8, 0x03, 0x03, 0x1C,
        0x00, 0xF4, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04,
        0x04, 0x01, 0x02, 0x1D, 0x00, 0xD9, 0x00, 0x00,
        0x00, 0x00, 0x00, 0xAE, 0x00, 0x00, 0x01, 0x1C,
        0x00, 0xB4, 0x01, 0x00, 0x00, 0x00, 0x00, 0x09,
        0x00, 0x80, 0x01, 0x00, 0xF4, 0x01, 0x00, 0x00,
        0x00,
    };

    DecodeStatus status = DecodeStatus::OK;
    auto decoded = decodeScene(kReportedFixture, sizeof(kReportedFixture), &status);
    TEST_ASSERT_NULL(decoded.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::BAD_VERSION), static_cast<int>(status));
}

// ═════════════════════════════════════════════════════════════════════
// Group 4 — Malformed inputs
// ═════════════════════════════════════════════════════════════════════

void test_decode_truncated_at_every_prefix() {
    WireBuilder full;
    full.header(0);
    full.record(PAT_NOISE_BASIC, [](WireBuilder &body) {
        body.sigConstant(550);
    });
    full.u8(0);

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
    full.header(1);
    full.record(PAT_FLOW_FIELD, [](WireBuilder &body) {
        body.u8(32)
            .u8(3)
            .u8(static_cast<uint8_t>(FlowFieldPattern::EmitterMode::Both));
        appendSignal(body, SIG_SINE_BOUNDED_PH);
        appendSignal(body, SIG_TRIANGLE_BOUNDED);
        appendSignal(body, SIG_SQUARE);
        appendSignal(body, SIG_SAWTOOTH_BOUNDED_PH);
        appendSignal(body, SIG_NOISE_BOUNDED_PH);
        appendSignal(body, SIG_SMAP);
        appendSignal(body, SIG_SCALE);
        appendSignal(body, SIG_QUADRATIC_OUT);
    });
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
    w.header(0xFE);    // unknown palette
    w.record(PAT_NOISE_BASIC, [](WireBuilder &body) {
        body.sigConstant(550);
    });
    w.u8(0);
    DecodeStatus status;
    auto s = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_NULL(s.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::BAD_ENUM), static_cast<int>(status));
}

void test_decode_life_variant_bad_rule() {
    WireBuilder w;
    w.header(0);
    w.record(PAT_RASTER_LIFE_VARIANT, [](WireBuilder &body) {
        body.u16(200).u16(0).u16(350).u8(3);   // rule 3 is out of range (0..2)
    });
    w.u8(0);
    DecodeStatus status = DecodeStatus::OK;
    auto s = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_NULL(s.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::BAD_ENUM), static_cast<int>(status));
}

void test_decode_unknown_pattern_tag() {
    WireBuilder w;
    w.header(0).record(0xEE, [](WireBuilder &) {});   // tag 0xEE is not a defined pattern
    DecodeStatus status;
    auto s = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_NULL(s.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::UNKNOWN_TAG), static_cast<int>(status));
}

void test_decode_unknown_signal_tag() {
    WireBuilder w;
    w.header(0);
    w.record(PAT_NOISE_BASIC, [](WireBuilder &body) {
        body.record(0xCC, [](WireBuilder &) {});   // 0xCC is not a defined signal
    });
    w.u8(0);
    DecodeStatus status;
    auto s = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_NULL(s.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::UNKNOWN_TAG), static_cast<int>(status));
}

void test_decode_unknown_transform_tag() {
    WireBuilder w;
    w.header(0);
    w.record(PAT_NOISE_BASIC, [](WireBuilder &body) {
        body.sigConstant(550);
    });
    w.u8(1).record(0xDD, [](WireBuilder &) {});
    DecodeStatus status;
    auto s = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_NULL(s.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::UNKNOWN_TAG), static_cast<int>(status));
}

void test_decode_rejects_legacy_palette_transform_tag() {
    WireBuilder w;
    w.header(0);
    w.record(PAT_NOISE_BASIC, [](WireBuilder &body) {
        body.sigConstant(550);
    });
    w.u8(1);
    w.record(LEGACY_TFM_PALETTE, [](WireBuilder &body) {
        body.sigConstant(100);
    });

    DecodeStatus status;
    auto s = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_NULL(s.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::UNKNOWN_TAG), static_cast<int>(status));
}

void test_decode_rejects_excessive_signal_nesting() {
    WireBuilder w;
    w.header(0);
    w.record(PAT_NOISE_BASIC, [](WireBuilder &body) {
        appendNestedSmapSignal(body, 65);
    });
    w.u8(0);

    DecodeStatus status = DecodeStatus::OK;
    auto s = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_NULL(s.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::BAD_ENUM), static_cast<int>(status));
}

void test_decode_bad_pattern_enum() {
    // TilingPattern with shape byte > HEXAGON (= 2) → BAD_ENUM.
    WireBuilder w;
    w.header(0);
    w.record(PAT_TILING, [](WireBuilder &body) {
        body.u16(32).u8(4).u8(0xFF);
    });
    w.u8(0);
    DecodeStatus status;
    auto s = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_NULL(s.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::BAD_ENUM), static_cast<int>(status));
}

void test_decode_bad_palette_tint_mode() {
    WireBuilder w;
    w.header(0);
    w.record(PAT_NOISE_BASIC, [](WireBuilder &body) {
        body.sigConstant(550);
    });
    w.u8(1);
    w.record(TFM_PALETTE_CLIP, [](WireBuilder &body) {
        body.u16(raw(perMil(250))).u8(0xFF);
    });

    DecodeStatus status;
    auto s = decodeScene(w.data(), w.size(), &status);
    TEST_ASSERT_NULL(s.get());
    TEST_ASSERT_EQUAL(static_cast<int>(DecodeStatus::BAD_ENUM), static_cast<int>(status));
}

// ═════════════════════════════════════════════════════════════════════
// .PDS display-spec codec
// ═════════════════════════════════════════════════════════════════════

namespace pds_test {
    // Little-endian byte helpers for crafting raw .PDS blobs inline.
    inline void put16(std::vector<uint8_t> &b, uint16_t v) {
        b.push_back(static_cast<uint8_t>(v & 0xFF));
        b.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    }
    inline void put32(std::vector<uint8_t> &b, uint32_t v) {
        b.push_back(static_cast<uint8_t>(v & 0xFF));
        b.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
        b.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
        b.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    }

    struct RawSection {
        uint8_t type;
        uint8_t flags;
        std::vector<uint8_t> body;
    };

    inline std::vector<uint8_t> frame(const std::vector<RawSection> &sections,
                                      uint8_t version = PDS_VERSION) {
        std::vector<uint8_t> b = {0x50, 0x44, 0x53, 0x00, version,
                                  static_cast<uint8_t>(sections.size())};
        for (const auto &s : sections) {
            b.push_back(s.type);
            b.push_back(s.flags);
            put32(b, static_cast<uint32_t>(s.body.size()));
            b.insert(b.end(), s.body.begin(), s.body.end());
        }
        return b;
    }

    inline std::vector<uint8_t> geometryBody(uint16_t ledCount) {
        std::vector<uint8_t> b;
        put16(b, ledCount);
        for (uint16_t i = 0; i < ledCount; ++i) {
            put16(b, static_cast<uint16_t>(i * 7));
            put16(b, static_cast<uint16_t>(i * 11));
        }
        return b;
    }

    inline std::vector<uint8_t> fastledBody() {
        std::vector<uint8_t> b;
        b.push_back(static_cast<uint8_t>(PdsBackendKind::FASTLED));
        put16(b, 4); // target_id len
        const char *id = "xiao";
        b.insert(b.end(), id, id + 4);
        b.push_back(0);          // chipset WS2812 (clockless)
        put16(b, 5);             // data_pin
        put16(b, 0xFFFF);        // clock_pin none
        b.push_back(2);          // rgb_order GRB
        b.push_back(200);        // brightness
        b.push_back(16);         // refresh_ms
        put32(b, 0);             // color_correction
        return b;
    }

    // Build a valid in-memory spec (polar-only FastLED).
    inline LoadedDisplaySpec makePolarFastLed(uint16_t ledCount) {
        LoadedDisplaySpec s;
        s.sourceKind = 0;
        s.name = "custom";
        for (uint16_t i = 0; i < ledCount; ++i) {
            s.leds.push_back({u0x16(static_cast<uint16_t>(i * 7)),
                              u0x16(static_cast<uint16_t>(i * 11))});
        }
        s.output.backend_kind = static_cast<uint8_t>(PdsBackendKind::FASTLED);
        s.output.fastled.target_id = "xiao";
        s.output.fastled.chipset = 0;
        s.output.fastled.data_pin = 5;
        s.output.fastled.clock_pin = 0xFFFF;
        s.output.fastled.rgb_order = 2;
        s.output.fastled.brightness = 200;
        s.output.fastled.refresh_ms = 16;
        return s;
    }

    // Dense raster SmartMatrix (128-style: (64*2)/2 = 64 → 64×64).
    inline LoadedDisplaySpec makeDenseSmartMatrix(uint16_t w, uint16_t h) {
        LoadedDisplaySpec s;
        s.name = "matrix";
        s.hasRaster = true;
        s.rasterWidth = w;
        s.rasterHeight = h;
        for (uint16_t y = 0; y < h; ++y) {
            for (uint16_t x = 0; x < w; ++x) {
                s.leds.push_back({u0x16(static_cast<uint16_t>(x)), u0x16(static_cast<uint16_t>(y))});
                s.rasterCells.push_back({x, y});
            }
        }
        s.output.backend_kind = static_cast<uint8_t>(PdsBackendKind::SMARTMATRIX);
        s.output.smartmatrix.target_id = "teensy41_matrix";
        s.output.smartmatrix.panel_width = 64;
        s.output.smartmatrix.panel_height = 64;
        s.output.smartmatrix.chain_width = 2;
        s.output.smartmatrix.chain_height = 2;
        s.output.smartmatrix.subsample = 2;
        s.output.smartmatrix.refresh_depth = 36;
        s.output.smartmatrix.dma_buffer_rows = 4;
        s.output.smartmatrix.panel_type = 2;
        return s;
    }
}  // namespace pds_test

static void expectReject(const std::vector<uint8_t> &bytes, DisplaySpecDecodeStatus want) {
    DisplaySpecDecodeStatus st = DisplaySpecDecodeStatus::OK;
    auto spec = decodeDisplaySpec(bytes.data(), bytes.size(), &st);
    TEST_ASSERT_NULL(spec.get());
    TEST_ASSERT_EQUAL(static_cast<int>(want), static_cast<int>(st));
}

void test_pds_roundtrip_polar_fastled() {
    LoadedDisplaySpec original = pds_test::makePolarFastLed(241);
    auto bytes = encodeDisplaySpec(original);
    DisplaySpecDecodeStatus st;
    auto decoded = decodeDisplaySpec(bytes.data(), bytes.size(), &st);
    TEST_ASSERT_EQUAL(static_cast<int>(DisplaySpecDecodeStatus::OK), static_cast<int>(st));
    TEST_ASSERT_NOT_NULL(decoded.get());
    TEST_ASSERT_EQUAL_UINT16(241, decoded->nbLeds());
    TEST_ASSERT_FALSE(decoded->hasRaster);
    for (uint16_t i = 0; i < 241; ++i) {
        TEST_ASSERT_EQUAL_UINT16(raw(original.leds[i].first), raw(decoded->leds[i].first));
        TEST_ASSERT_EQUAL_UINT16(raw(original.leds[i].second), raw(decoded->leds[i].second));
    }
    TEST_ASSERT_EQUAL(static_cast<int>(PdsBackendKind::FASTLED),
                      static_cast<int>(decoded->output.backend_kind));
    TEST_ASSERT_EQUAL_UINT16(5, decoded->output.fastled.data_pin);
    TEST_ASSERT_EQUAL_UINT8(200, decoded->output.fastled.brightness);
    // Re-encode is byte identical.
    auto bytes2 = encodeDisplaySpec(*decoded);
    TEST_ASSERT_EQUAL_UINT32(bytes.size(), bytes2.size());
    TEST_ASSERT_EQUAL_MEMORY(bytes.data(), bytes2.data(), bytes.size());
}

void test_pds_roundtrip_raster_smartmatrix() {
    LoadedDisplaySpec original = pds_test::makeDenseSmartMatrix(64, 64);
    auto bytes = encodeDisplaySpec(original);
    DisplaySpecDecodeStatus st;
    auto decoded = decodeDisplaySpec(bytes.data(), bytes.size(), &st);
    TEST_ASSERT_EQUAL(static_cast<int>(DisplaySpecDecodeStatus::OK), static_cast<int>(st));
    TEST_ASSERT_NOT_NULL(decoded.get());
    TEST_ASSERT_TRUE(decoded->hasRaster);
    TEST_ASSERT_EQUAL_UINT16(64, decoded->rasterWidth);
    TEST_ASSERT_EQUAL_UINT16(64, decoded->rasterHeight);
    TEST_ASSERT_EQUAL_UINT16(4096, decoded->nbLeds());
    // A raster render point carries valid raster coords.
    RenderPoint rp = decoded->toRenderPoint(65);
    TEST_ASSERT_TRUE(rp.raster.valid);
    TEST_ASSERT_EQUAL_UINT16(1, rp.raster.x);
    TEST_ASSERT_EQUAL_UINT16(1, rp.raster.y);
}

void test_pds_web_geometry_raster_grid() {
    LoadedDisplaySpec spec = pds_test::makeDenseSmartMatrix(8, 4);
    WebDisplayGeometry geo = buildWebGeometry(spec);
    TEST_ASSERT_EQUAL_UINT32(32, geo.points.size());
    // First cell at origin, laid out on the rectangular grid (not polar).
    TEST_ASSERT_EQUAL_FLOAT(0.0f, geo.points[0].x);
    TEST_ASSERT_EQUAL_FLOAT(0.0f, geo.points[0].y);
    TEST_ASSERT_EQUAL_FLOAT(3.5f, geo.centerX);
    TEST_ASSERT_EQUAL_FLOAT(1.5f, geo.centerY);
}

void test_pds_reject_bad_magic() {
    auto bytes = encodeDisplaySpec(pds_test::makePolarFastLed(4));
    bytes[0] = 'X';
    expectReject(bytes, DisplaySpecDecodeStatus::BAD_MAGIC);
}

void test_pds_reject_bad_version() {
    auto bytes = encodeDisplaySpec(pds_test::makePolarFastLed(4));
    bytes[4] = 0x02;
    expectReject(bytes, DisplaySpecDecodeStatus::BAD_VERSION);
}

void test_pds_reject_trailing_bytes() {
    auto bytes = encodeDisplaySpec(pds_test::makePolarFastLed(4));
    bytes.push_back(0xAB);
    expectReject(bytes, DisplaySpecDecodeStatus::TRAILING_BYTES);
}

void test_pds_reject_missing_geometry() {
    using namespace pds_test;
    auto bytes = frame({{0x01, PDS_SECTION_FLAG_CRITICAL, {0, 0}}});
    expectReject(bytes, DisplaySpecDecodeStatus::MISSING_GEOMETRY);
}

void test_pds_reject_empty_geometry() {
    using namespace pds_test;
    auto bytes = frame({{0x02, PDS_SECTION_FLAG_CRITICAL, geometryBody(0)}});
    expectReject(bytes, DisplaySpecDecodeStatus::EMPTY_GEOMETRY);
}

void test_pds_reject_dup_section() {
    using namespace pds_test;
    auto bytes = frame({{0x02, PDS_SECTION_FLAG_CRITICAL, geometryBody(2)},
                        {0x02, PDS_SECTION_FLAG_CRITICAL, geometryBody(2)}});
    expectReject(bytes, DisplaySpecDecodeStatus::DUP_SECTION);
}

void test_pds_reject_section_order() {
    using namespace pds_test;
    auto bytes = frame({{0x04, PDS_SECTION_FLAG_CRITICAL, fastledBody()},
                        {0x02, PDS_SECTION_FLAG_CRITICAL, geometryBody(2)}});
    expectReject(bytes, DisplaySpecDecodeStatus::BAD_SECTION_ORDER);
}

void test_pds_reject_unknown_critical_section() {
    using namespace pds_test;
    auto bytes = frame({{0x02, PDS_SECTION_FLAG_CRITICAL, geometryBody(2)},
                        {0x7F, PDS_SECTION_FLAG_CRITICAL, {1, 2, 3}}});
    expectReject(bytes, DisplaySpecDecodeStatus::UNKNOWN_CRITICAL_SECTION);
}

void test_pds_reject_bad_section_flags() {
    using namespace pds_test;
    auto bytes = frame({{0x02, 0x02 /* reserved bit set */, geometryBody(2)}});
    expectReject(bytes, DisplaySpecDecodeStatus::BAD_SECTION_FLAGS);
}

void test_pds_reject_section_trailing_bytes() {
    using namespace pds_test;
    auto body = geometryBody(2);
    body.push_back(0x99); // extra byte inside a known section body
    auto bytes = frame({{0x02, PDS_SECTION_FLAG_CRITICAL, body}});
    expectReject(bytes, DisplaySpecDecodeStatus::SECTION_TRAILING_BYTES);
}

void test_pds_accept_unknown_noncritical_positions() {
    using namespace pds_test;
    RawSection unk{0x7F, 0x00, {9, 9, 9}};
    RawSection geo{0x02, PDS_SECTION_FLAG_CRITICAL, geometryBody(3)};
    RawSection out{0x04, PDS_SECTION_FLAG_CRITICAL, fastledBody()};
    // before, between, after known sections
    for (const auto &blob : {frame({unk, geo, out}),
                             frame({geo, unk, out}),
                             frame({geo, out, unk})}) {
        DisplaySpecDecodeStatus st;
        auto spec = decodeDisplaySpec(blob.data(), blob.size(), &st);
        TEST_ASSERT_EQUAL(static_cast<int>(DisplaySpecDecodeStatus::OK), static_cast<int>(st));
        TEST_ASSERT_NOT_NULL(spec.get());
        TEST_ASSERT_EQUAL_UINT16(3, spec->nbLeds());
    }
}

void test_pds_absent_output_is_none() {
    using namespace pds_test;
    auto bytes = frame({{0x02, PDS_SECTION_FLAG_CRITICAL, geometryBody(2)}});
    DisplaySpecDecodeStatus st;
    auto spec = decodeDisplaySpec(bytes.data(), bytes.size(), &st);
    TEST_ASSERT_EQUAL(static_cast<int>(DisplaySpecDecodeStatus::OK), static_cast<int>(st));
    TEST_ASSERT_NOT_NULL(spec.get());
    TEST_ASSERT_EQUAL(static_cast<int>(PdsBackendKind::NONE),
                      static_cast<int>(spec->output.backend_kind));
}

void test_pds_reject_bad_output_enum() {
    using namespace pds_test;
    auto body = fastledBody();
    body[7] = 99; // chipset byte (after backend_kind + u16 len + 4 name bytes)
    auto bytes = frame({{0x02, PDS_SECTION_FLAG_CRITICAL, geometryBody(2)},
                        {0x04, PDS_SECTION_FLAG_CRITICAL, body}});
    expectReject(bytes, DisplaySpecDecodeStatus::BAD_OUTPUT_ENUM);
}

void test_pds_reject_fastled_no_data_pin() {
    using namespace pds_test;
    auto body = fastledBody();
    // data_pin occupies bytes [8],[9] (backend(1)+len(2)+name(4)+chipset(1)).
    body[8] = 0xFF;
    body[9] = 0xFF;
    auto bytes = frame({{0x02, PDS_SECTION_FLAG_CRITICAL, geometryBody(2)},
                        {0x04, PDS_SECTION_FLAG_CRITICAL, body}});
    expectReject(bytes, DisplaySpecDecodeStatus::BAD_OUTPUT_CONFIG);
}

void test_pds_unknown_backend_roundtrip_identical() {
    using namespace pds_test;
    std::vector<uint8_t> outBody;
    outBody.push_back(7); // unknown backend_kind
    const uint8_t raw[] = {0xDE, 0xAD, 0xBE, 0xEF};
    outBody.insert(outBody.end(), raw, raw + 4);
    auto bytes = frame({{0x02, PDS_SECTION_FLAG_CRITICAL, geometryBody(2)},
                        {0x04, PDS_SECTION_FLAG_CRITICAL, outBody}});
    DisplaySpecDecodeStatus st;
    auto spec = decodeDisplaySpec(bytes.data(), bytes.size(), &st);
    TEST_ASSERT_EQUAL(static_cast<int>(DisplaySpecDecodeStatus::OK), static_cast<int>(st));
    TEST_ASSERT_NOT_NULL(spec.get());
    TEST_ASSERT_EQUAL_UINT8(7, spec->output.backend_kind);
    TEST_ASSERT_EQUAL_UINT32(4, spec->output.rawBody.size());
    auto reencoded = encodeDisplaySpec(*spec);
    TEST_ASSERT_EQUAL_UINT32(bytes.size(), reencoded.size());
    TEST_ASSERT_EQUAL_MEMORY(bytes.data(), reencoded.data(), bytes.size());
}

void test_pds_deployability() {
    // Dense SmartMatrix + FastLED are deployable.
    LoadedDisplaySpec sm = pds_test::makeDenseSmartMatrix(8, 4);
    TEST_ASSERT_EQUAL(static_cast<int>(DeployStatus::DEPLOYABLE),
                      static_cast<int>(validateDeployability(sm)));
    LoadedDisplaySpec fl = pds_test::makePolarFastLed(10);
    TEST_ASSERT_EQUAL(static_cast<int>(DeployStatus::DEPLOYABLE),
                      static_cast<int>(validateDeployability(fl)));
    // NONE backend → not deployable, but still decodes.
    LoadedDisplaySpec none = pds_test::makePolarFastLed(10);
    none.output.backend_kind = static_cast<uint8_t>(PdsBackendKind::NONE);
    TEST_ASSERT_EQUAL(static_cast<int>(DeployStatus::NO_BACKEND),
                      static_cast<int>(validateDeployability(none)));
    // SmartMatrix without dense raster → not deployable.
    LoadedDisplaySpec sparse = pds_test::makeDenseSmartMatrix(8, 4);
    sparse.rasterCells[0] = sparse.rasterCells[1]; // duplicate cell
    TEST_ASSERT_EQUAL(static_cast<int>(DeployStatus::RASTER_NOT_DENSE),
                      static_cast<int>(validateDeployability(sparse)));
    // Unknown backend → unsupported.
    LoadedDisplaySpec unk = pds_test::makePolarFastLed(10);
    unk.output.backend_kind = 7;
    TEST_ASSERT_EQUAL(static_cast<int>(DeployStatus::UNSUPPORTED_BACKEND),
                      static_cast<int>(validateDeployability(unk)));
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
    RUN_TEST(test_decode_noise_basic_loop_golden);
    RUN_TEST(test_decode_noise_basic_loop_rejects_zero_period);
    RUN_TEST(test_decode_all_signal_tags_compile);
    RUN_TEST(test_decode_all_pattern_tags_compile);
    RUN_TEST(test_decode_legacy_palette_glow_without_speed_signal);
    RUN_TEST(test_decode_palette_glow_without_tile_scale_signal);
    RUN_TEST(test_decode_uncovered_transform_tags_compile);
    RUN_TEST(test_decode_raster_conway_allows_palette_transform);
    RUN_TEST(test_decode_raster_conway_compiles_with_raster_display);
    RUN_TEST(test_decode_raster_conway_rejects_uv_transform);
    RUN_TEST(test_decode_scene_with_duration_overrides_default);
    RUN_TEST(test_embedded_psc_playlist_provider_decodes_scene);
    RUN_TEST(test_embedded_psc_playlist_provider_falls_back_after_decode_fail);
    RUN_TEST(test_embedded_psc_playlist_provider_has_builtin_fallback);
    RUN_TEST(test_decode_golden_fixture);
    RUN_TEST(test_decode_v1_length_prefixed_fixture);
    RUN_TEST(test_decode_js_generated_lockstep_fixtures);
    RUN_TEST(test_decode_v1_rejects_trailing_pattern_body_bytes);
    RUN_TEST(test_decode_v1_rejects_trailing_signal_body_bytes);
    RUN_TEST(test_decode_v1_rejects_trailing_transform_body_bytes);
    RUN_TEST(test_decode_v1_rejects_trailing_scene_bytes);
    RUN_TEST(test_decode_rejects_legacy_reported_fixture);
    RUN_TEST(test_decode_truncated_at_every_prefix);
    RUN_TEST(test_decode_complex_truncated_at_every_prefix);
    RUN_TEST(test_decode_bad_magic);
    RUN_TEST(test_decode_bad_version);
    RUN_TEST(test_decode_bad_palette_id);
    RUN_TEST(test_decode_life_variant_bad_rule);
    RUN_TEST(test_decode_unknown_pattern_tag);
    RUN_TEST(test_decode_unknown_signal_tag);
    RUN_TEST(test_decode_unknown_transform_tag);
    RUN_TEST(test_decode_rejects_legacy_palette_transform_tag);
    RUN_TEST(test_decode_rejects_excessive_signal_nesting);
    RUN_TEST(test_decode_bad_pattern_enum);
    RUN_TEST(test_decode_bad_palette_tint_mode);
    RUN_TEST(test_pds_roundtrip_polar_fastled);
    RUN_TEST(test_pds_roundtrip_raster_smartmatrix);
    RUN_TEST(test_pds_web_geometry_raster_grid);
    RUN_TEST(test_pds_reject_bad_magic);
    RUN_TEST(test_pds_reject_bad_version);
    RUN_TEST(test_pds_reject_trailing_bytes);
    RUN_TEST(test_pds_reject_missing_geometry);
    RUN_TEST(test_pds_reject_empty_geometry);
    RUN_TEST(test_pds_reject_dup_section);
    RUN_TEST(test_pds_reject_section_order);
    RUN_TEST(test_pds_reject_unknown_critical_section);
    RUN_TEST(test_pds_reject_bad_section_flags);
    RUN_TEST(test_pds_reject_section_trailing_bytes);
    RUN_TEST(test_pds_accept_unknown_noncritical_positions);
    RUN_TEST(test_pds_absent_output_is_none);
    RUN_TEST(test_pds_reject_bad_output_enum);
    RUN_TEST(test_pds_reject_fastled_no_data_pin);
    RUN_TEST(test_pds_unknown_backend_roundtrip_identical);
    RUN_TEST(test_pds_deployability);
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
    RUN_TEST(test_decode_noise_basic_loop_golden);
    RUN_TEST(test_decode_noise_basic_loop_rejects_zero_period);
    RUN_TEST(test_decode_all_signal_tags_compile);
    RUN_TEST(test_decode_all_pattern_tags_compile);
    RUN_TEST(test_decode_legacy_palette_glow_without_speed_signal);
    RUN_TEST(test_decode_palette_glow_without_tile_scale_signal);
    RUN_TEST(test_decode_uncovered_transform_tags_compile);
    RUN_TEST(test_decode_raster_conway_allows_palette_transform);
    RUN_TEST(test_decode_raster_conway_compiles_with_raster_display);
    RUN_TEST(test_decode_raster_conway_rejects_uv_transform);
    RUN_TEST(test_decode_scene_with_duration_overrides_default);
    RUN_TEST(test_embedded_psc_playlist_provider_decodes_scene);
    RUN_TEST(test_embedded_psc_playlist_provider_falls_back_after_decode_fail);
    RUN_TEST(test_embedded_psc_playlist_provider_has_builtin_fallback);
    RUN_TEST(test_decode_golden_fixture);
    RUN_TEST(test_decode_v1_length_prefixed_fixture);
    RUN_TEST(test_decode_js_generated_lockstep_fixtures);
    RUN_TEST(test_decode_v1_rejects_trailing_pattern_body_bytes);
    RUN_TEST(test_decode_v1_rejects_trailing_signal_body_bytes);
    RUN_TEST(test_decode_v1_rejects_trailing_transform_body_bytes);
    RUN_TEST(test_decode_v1_rejects_trailing_scene_bytes);
    RUN_TEST(test_decode_rejects_legacy_reported_fixture);
    RUN_TEST(test_decode_truncated_at_every_prefix);
    RUN_TEST(test_decode_complex_truncated_at_every_prefix);
    RUN_TEST(test_decode_bad_magic);
    RUN_TEST(test_decode_bad_version);
    RUN_TEST(test_decode_bad_palette_id);
    RUN_TEST(test_decode_life_variant_bad_rule);
    RUN_TEST(test_decode_unknown_pattern_tag);
    RUN_TEST(test_decode_unknown_signal_tag);
    RUN_TEST(test_decode_unknown_transform_tag);
    RUN_TEST(test_decode_rejects_legacy_palette_transform_tag);
    RUN_TEST(test_decode_rejects_excessive_signal_nesting);
    RUN_TEST(test_decode_bad_pattern_enum);
    RUN_TEST(test_decode_bad_palette_tint_mode);
    RUN_TEST(test_pds_roundtrip_polar_fastled);
    RUN_TEST(test_pds_roundtrip_raster_smartmatrix);
    RUN_TEST(test_pds_web_geometry_raster_grid);
    RUN_TEST(test_pds_reject_bad_magic);
    RUN_TEST(test_pds_reject_bad_version);
    RUN_TEST(test_pds_reject_trailing_bytes);
    RUN_TEST(test_pds_reject_missing_geometry);
    RUN_TEST(test_pds_reject_empty_geometry);
    RUN_TEST(test_pds_reject_dup_section);
    RUN_TEST(test_pds_reject_section_order);
    RUN_TEST(test_pds_reject_unknown_critical_section);
    RUN_TEST(test_pds_reject_bad_section_flags);
    RUN_TEST(test_pds_reject_section_trailing_bytes);
    RUN_TEST(test_pds_accept_unknown_noncritical_positions);
    RUN_TEST(test_pds_absent_output_is_none);
    RUN_TEST(test_pds_reject_bad_output_enum);
    RUN_TEST(test_pds_reject_fastled_no_data_pin);
    RUN_TEST(test_pds_unknown_backend_roundtrip_identical);
    RUN_TEST(test_pds_deployability);
    return UNITY_END();
}
#endif
