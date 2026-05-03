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

// PSC v0 wire-format decoder. See SceneCodec.h for the format spec.
//
// Tag tables in this file MUST match web/sketches/composer/schema.js
// byte-for-byte. The cross-implementation golden fixture in test_composer
// is the safety net.

#include "composer/SceneCodec.h"
#include "composer/PaletteTable.h"

#include "renderer/scene/Scene.h"
#include "renderer/layer/Layer.h"
#include "renderer/layer/LayerBuilder.h"
#include "renderer/pipeline/patterns/Patterns.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/transforms/RotationTransform.h"
#include "renderer/pipeline/transforms/TranslationTransform.h"
#include "renderer/pipeline/transforms/ZoomTransform.h"
#include "renderer/pipeline/transforms/VortexTransform.h"
#include "renderer/pipeline/transforms/KaleidoscopeTransform.h"
#include "renderer/pipeline/transforms/RadialKaleidoscopeTransform.h"
#include "renderer/pipeline/transforms/PaletteTransform.h"
#include "renderer/pipeline/transforms/TilingTransform.h"
#include "renderer/pipeline/transforms/FlowFieldTransform.h"

#include <cstring>
#include <utility>

namespace PolarShader::composer {
    namespace {

        // ───── Tag tables (mirror schema.js) ──────────────────────────

        // Signal tags. Leaves: 0x00-0x0F. Modulators: 0x10-0x20.
        enum SignalTag : uint8_t {
            SIG_CONSTANT          = 0x00, // body: u16 permille (0..65535)
            SIG_C_RANDOM          = 0x01, // body: (none)
            SIG_LINEAR            = 0x02, // body: u32 durationMs + u8 loopMode
            SIG_QUADRATIC_IN      = 0x03, // body: u32 durationMs + u8 loopMode
            SIG_QUADRATIC_OUT     = 0x04, // body: u32 durationMs + u8 loopMode
            SIG_QUADRATIC_IN_OUT  = 0x05, // body: u32 durationMs + u8 loopMode

            SIG_SINE              = 0x10, // body: signal phaseVelocity + i32 phaseOffset(sf16)
            SIG_SINE_BOUNDED      = 0x11, // body: signal phaseVelocity + signal floor + signal ceiling
            SIG_SINE_BOUNDED_PH   = 0x12, // body: signal phaseVelocity + i32 phaseOffset + signal floor + signal ceiling
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
            SIG_SMAP              = 0x1F, // body: signal signal + signal floor + signal ceiling
            SIG_SCALE             = 0x20, // body: signal signal + u16 factor (f16)
        };

        // Pattern tags. NoisePattern is split into 4 tags (one per NoiseType).
        enum PatternTag : uint8_t {
            PAT_NOISE_BASIC       = 0x00, // body: signal depthSpeedSignal
            PAT_NOISE_FBM         = 0x01, // body: u8 octaves
            PAT_NOISE_TURBULENCE  = 0x02, // body: (none)
            PAT_NOISE_RIDGED      = 0x03, // body: (none)
            PAT_TILING            = 0x04, // body: u16 cellSize + u8 colorCount + u8 shape
            PAT_REACTION_DIFFUSION= 0x05, // body: u8 preset + u8 width + u8 height + u8 stepsPerFrame
            PAT_FLOW_FIELD        = 0x06, // body: u8 gridSize + u8 dotCount + u8 mode + 8 signals
            PAT_TRANSPORT         = 0x07, // body: u8 gridSize + u8 mode + u8 velocityGlow + 4 signals
            PAT_SPIRAL            = 0x08, // body: u8 armCount + u8 clockwise + 3 signals
            PAT_ANNULI            = 0x09, // body: u8 ringCount + u8 slicesPerRing + u8 reverse + u16 stepIntervalMs + u16 holdMs
            PAT_FLURRY            = 0x0A, // body: u8 gridSize + u8 lineCount + u8 shape + 6 signals
            PAT_WORLEY            = 0x0B, // body: i32 cellSize raw + u8 aliasingMode
            PAT_VORONOI           = 0x0C, // body: i32 cellSize raw + u8 aliasingMode
        };

        enum TransformTag : uint8_t {
            TFM_ROTATION          = 0x00, // body: u8 isAngleTurn + signal angle
            TFM_TRANSLATION       = 0x01, // body: signal direction + signal speed
            TFM_ZOOM              = 0x02, // body: signal scale
            TFM_VORTEX            = 0x03, // body: signal strength
            TFM_KALEIDOSCOPE      = 0x04, // body: u8 nbFacets + u8 isMirrored
            TFM_RADIAL_KALEIDO    = 0x05, // body: u16 radialDivisions + u8 isMirrored
            TFM_PALETTE           = 0x06, // body: signal offset (offset-only PaletteTransform)
            TFM_TILING            = 0x07, // body: u8 mirrored + u8 shape + signal cellSize
            TFM_FLOW_FIELD        = 0x08, // body: 4 signals (phaseSpeed, flowStrength, fieldScale, maxOffset)
        };

        // ───── Bounds-checked byte reader ─────────────────────────────

        class ByteReader {
        public:
            ByteReader(const uint8_t *data, std::size_t len) : data_(data), len_(len), pos_(0), bad_(false) {}

            std::size_t remaining() const { return bad_ ? 0 : (len_ - pos_); }
            bool ok() const { return !bad_; }
            void fail() { bad_ = true; }

            uint8_t readU8() {
                if (remaining() < 1) { bad_ = true; return 0; }
                return data_[pos_++];
            }

            uint16_t readU16() {
                if (remaining() < 2) { bad_ = true; return 0; }
                uint16_t v = static_cast<uint16_t>(data_[pos_]) |
                             (static_cast<uint16_t>(data_[pos_ + 1]) << 8);
                pos_ += 2;
                return v;
            }

            uint32_t readU32() {
                if (remaining() < 4) { bad_ = true; return 0; }
                uint32_t v = static_cast<uint32_t>(data_[pos_]) |
                             (static_cast<uint32_t>(data_[pos_ + 1]) << 8) |
                             (static_cast<uint32_t>(data_[pos_ + 2]) << 16) |
                             (static_cast<uint32_t>(data_[pos_ + 3]) << 24);
                pos_ += 4;
                return v;
            }

            int32_t readI32() {
                uint32_t u = readU32();
                int32_t v;
                std::memcpy(&v, &u, sizeof(v));
                return v;
            }

        private:
            const uint8_t *data_;
            std::size_t len_;
            std::size_t pos_;
            bool bad_;
        };

        // ───── Helpers for status propagation ─────────────────────────

        struct DecodeError {
            DecodeStatus status;
        };

        void setStatusIfOk(DecodeStatus *out, DecodeStatus s) {
            if (out && *out == DecodeStatus::OK) *out = s;
        }

        // ───── Recursive Signal decoder ───────────────────────────────

        Sf16Signal decodeSignal(ByteReader &r, DecodeStatus *status) {
            if (*status != DecodeStatus::OK) return Sf16Signal();
            uint8_t tag = r.readU8();
            if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return Sf16Signal(); }

            switch (tag) {
                case SIG_CONSTANT: {
                    uint16_t permille = r.readU16();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return Sf16Signal(); }
                    return constant(permille);
                }
                case SIG_C_RANDOM:
                    return cRandom();
                case SIG_LINEAR:
                case SIG_QUADRATIC_IN:
                case SIG_QUADRATIC_OUT:
                case SIG_QUADRATIC_IN_OUT: {
                    TimeMillis duration = r.readU32();
                    uint8_t loopByte = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return Sf16Signal(); }
                    if (loopByte > static_cast<uint8_t>(LoopMode::SATURATE)) {
                        setStatusIfOk(status, DecodeStatus::BAD_ENUM); return Sf16Signal();
                    }
                    LoopMode loop = static_cast<LoopMode>(loopByte);
                    switch (tag) {
                        case SIG_LINEAR:           return linear(duration, loop);
                        case SIG_QUADRATIC_IN:     return quadraticIn(duration, loop);
                        case SIG_QUADRATIC_OUT:    return quadraticOut(duration, loop);
                        case SIG_QUADRATIC_IN_OUT: return quadraticInOut(duration, loop);
                    }
                    return Sf16Signal();
                }

                case SIG_SINE:
                case SIG_TRIANGLE:
                case SIG_SQUARE:
                case SIG_SAWTOOTH:
                case SIG_NOISE: {
                    Sf16Signal pv = decodeSignal(r, status);
                    int32_t phaseRaw = r.readI32();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return Sf16Signal(); }
                    if (*status != DecodeStatus::OK) return Sf16Signal();
                    sf16 phase = sf16(phaseRaw);
                    switch (tag) {
                        case SIG_SINE:     return sine(std::move(pv), phase);
                        case SIG_TRIANGLE: return triangle(std::move(pv), phase);
                        case SIG_SQUARE:   return square(std::move(pv), phase);
                        case SIG_SAWTOOTH: return sawtooth(std::move(pv), phase);
                        case SIG_NOISE:    return noise(std::move(pv), phase);
                    }
                    return Sf16Signal();
                }

                case SIG_SINE_BOUNDED:
                case SIG_TRIANGLE_BOUNDED:
                case SIG_SQUARE_BOUNDED:
                case SIG_SAWTOOTH_BOUNDED:
                case SIG_NOISE_BOUNDED: {
                    Sf16Signal pv      = decodeSignal(r, status);
                    Sf16Signal floorS  = decodeSignal(r, status);
                    Sf16Signal ceilS   = decodeSignal(r, status);
                    if (*status != DecodeStatus::OK) return Sf16Signal();
                    switch (tag) {
                        case SIG_SINE_BOUNDED:     return sine(std::move(pv), std::move(floorS), std::move(ceilS));
                        case SIG_TRIANGLE_BOUNDED: return triangle(std::move(pv), std::move(floorS), std::move(ceilS));
                        case SIG_SQUARE_BOUNDED:   return square(std::move(pv), std::move(floorS), std::move(ceilS));
                        case SIG_SAWTOOTH_BOUNDED: return sawtooth(std::move(pv), std::move(floorS), std::move(ceilS));
                        case SIG_NOISE_BOUNDED:    return noise(std::move(pv), std::move(floorS), std::move(ceilS));
                    }
                    return Sf16Signal();
                }

                case SIG_SINE_BOUNDED_PH:
                case SIG_TRIANGLE_BOUNDED_PH:
                case SIG_SQUARE_BOUNDED_PH:
                case SIG_SAWTOOTH_BOUNDED_PH:
                case SIG_NOISE_BOUNDED_PH: {
                    Sf16Signal pv = decodeSignal(r, status);
                    int32_t phaseRaw = r.readI32();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return Sf16Signal(); }
                    Sf16Signal floorS = decodeSignal(r, status);
                    Sf16Signal ceilS  = decodeSignal(r, status);
                    if (*status != DecodeStatus::OK) return Sf16Signal();
                    sf16 phase = sf16(phaseRaw);
                    switch (tag) {
                        case SIG_SINE_BOUNDED_PH:     return sine(std::move(pv), phase, std::move(floorS), std::move(ceilS));
                        case SIG_TRIANGLE_BOUNDED_PH: return triangle(std::move(pv), phase, std::move(floorS), std::move(ceilS));
                        case SIG_SQUARE_BOUNDED_PH:   return square(std::move(pv), phase, std::move(floorS), std::move(ceilS));
                        case SIG_SAWTOOTH_BOUNDED_PH: return sawtooth(std::move(pv), phase, std::move(floorS), std::move(ceilS));
                        case SIG_NOISE_BOUNDED_PH:    return noise(std::move(pv), phase, std::move(floorS), std::move(ceilS));
                    }
                    return Sf16Signal();
                }

                case SIG_SMAP: {
                    Sf16Signal s     = decodeSignal(r, status);
                    Sf16Signal floor = decodeSignal(r, status);
                    Sf16Signal ceil  = decodeSignal(r, status);
                    if (*status != DecodeStatus::OK) return Sf16Signal();
                    return smap(std::move(s), std::move(floor), std::move(ceil));
                }

                case SIG_SCALE: {
                    Sf16Signal s = decodeSignal(r, status);
                    uint16_t factorRaw = r.readU16();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return Sf16Signal(); }
                    if (*status != DecodeStatus::OK) return Sf16Signal();
                    return scale(std::move(s), f16(factorRaw));
                }

                default:
                    setStatusIfOk(status, DecodeStatus::UNKNOWN_TAG);
                    return Sf16Signal();
            }
        }

        // ───── Pattern decoder ─────────────────────────────────────────

        std::unique_ptr<UVPattern> decodePattern(ByteReader &r, DecodeStatus *status) {
            if (*status != DecodeStatus::OK) return nullptr;
            uint8_t tag = r.readU8();
            if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }

            switch (tag) {
                case PAT_NOISE_BASIC: {
                    Sf16Signal depth = decodeSignal(r, status);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return noisePattern(std::move(depth));
                }
                case PAT_NOISE_FBM: {
                    uint8_t octaves = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    return fbmNoisePattern(octaves);
                }
                case PAT_NOISE_TURBULENCE:
                    return turbulenceNoisePattern();
                case PAT_NOISE_RIDGED:
                    return ridgedNoisePattern();

                case PAT_TILING: {
                    uint16_t cellSize  = r.readU16();
                    uint8_t  colorCnt  = r.readU8();
                    uint8_t  shapeByte = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    if (shapeByte > static_cast<uint8_t>(TilingPattern::TileShape::HEXAGON)) {
                        setStatusIfOk(status, DecodeStatus::BAD_ENUM); return nullptr;
                    }
                    return tilingPattern(cellSize, colorCnt, static_cast<TilingPattern::TileShape>(shapeByte));
                }

                case PAT_REACTION_DIFFUSION: {
                    uint8_t presetByte = r.readU8();
                    uint8_t width = r.readU8();
                    uint8_t height = r.readU8();
                    uint8_t steps = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    if (presetByte > static_cast<uint8_t>(ReactionDiffusionPattern::Preset::Worms)) {
                        setStatusIfOk(status, DecodeStatus::BAD_ENUM); return nullptr;
                    }
                    return reactionDiffusionPattern(
                        static_cast<ReactionDiffusionPattern::Preset>(presetByte),
                        width, height, steps);
                }

                case PAT_FLOW_FIELD: {
                    uint8_t gridSize = r.readU8();
                    uint8_t dotCount = r.readU8();
                    uint8_t modeByte = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    if (modeByte > static_cast<uint8_t>(FlowFieldPattern::EmitterMode::Both)) {
                        setStatusIfOk(status, DecodeStatus::BAD_ENUM); return nullptr;
                    }
                    Sf16Signal xDrift   = decodeSignal(r, status);
                    Sf16Signal yDrift   = decodeSignal(r, status);
                    Sf16Signal amp      = decodeSignal(r, status);
                    Sf16Signal freq     = decodeSignal(r, status);
                    Sf16Signal endSpeed = decodeSignal(r, status);
                    Sf16Signal halfLife = decodeSignal(r, status);
                    Sf16Signal orbSpeed = decodeSignal(r, status);
                    Sf16Signal orbRad   = decodeSignal(r, status);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return flowFieldPattern(gridSize, dotCount,
                        static_cast<FlowFieldPattern::EmitterMode>(modeByte),
                        std::move(xDrift), std::move(yDrift), std::move(amp), std::move(freq),
                        std::move(endSpeed), std::move(halfLife), std::move(orbSpeed), std::move(orbRad));
                }

                case PAT_TRANSPORT: {
                    uint8_t gridSize = r.readU8();
                    uint8_t modeByte = r.readU8();
                    uint8_t glowByte = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    if (modeByte > static_cast<uint8_t>(TransportPattern::TransportMode::AttractorField)) {
                        setStatusIfOk(status, DecodeStatus::BAD_ENUM); return nullptr;
                    }
                    Sf16Signal radial   = decodeSignal(r, status);
                    Sf16Signal angular  = decodeSignal(r, status);
                    Sf16Signal halfLife = decodeSignal(r, status);
                    Sf16Signal emitter  = decodeSignal(r, status);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return transportPattern(gridSize,
                        static_cast<TransportPattern::TransportMode>(modeByte),
                        std::move(radial), std::move(angular), std::move(halfLife), std::move(emitter),
                        glowByte != 0);
                }

                case PAT_SPIRAL: {
                    uint8_t arms = r.readU8();
                    uint8_t cwByte = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    Sf16Signal tightness = decodeSignal(r, status);
                    Sf16Signal armThick  = decodeSignal(r, status);
                    Sf16Signal rotSpeed  = decodeSignal(r, status);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return spiralPattern(arms, cwByte != 0,
                        std::move(tightness), std::move(armThick), std::move(rotSpeed));
                }

                case PAT_ANNULI: {
                    uint8_t  rings    = r.readU8();
                    uint8_t  slices   = r.readU8();
                    uint8_t  revByte  = r.readU8();
                    uint16_t stepMs   = r.readU16();
                    uint16_t holdMs   = r.readU16();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    return annuliPattern(rings, slices, revByte != 0, stepMs, holdMs);
                }

                case PAT_FLURRY: {
                    uint8_t gridSize  = r.readU8();
                    uint8_t lineCount = r.readU8();
                    uint8_t shapeByte = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    if (shapeByte > static_cast<uint8_t>(FlurryPattern::Shape::Ball)) {
                        setStatusIfOk(status, DecodeStatus::BAD_ENUM); return nullptr;
                    }
                    Sf16Signal xDrift   = decodeSignal(r, status);
                    Sf16Signal yDrift   = decodeSignal(r, status);
                    Sf16Signal amp      = decodeSignal(r, status);
                    Sf16Signal freq     = decodeSignal(r, status);
                    Sf16Signal endSpeed = decodeSignal(r, status);
                    Sf16Signal fade     = decodeSignal(r, status);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return flurryPattern(gridSize, lineCount,
                        static_cast<FlurryPattern::Shape>(shapeByte),
                        std::move(xDrift), std::move(yDrift), std::move(amp), std::move(freq),
                        std::move(endSpeed), std::move(fade));
                }

                case PAT_WORLEY:
                case PAT_VORONOI: {
                    int32_t cellRaw = r.readI32();
                    uint8_t aliasByte = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    if (aliasByte > static_cast<uint8_t>(WorleyAliasing::Precise)) {
                        setStatusIfOk(status, DecodeStatus::BAD_ENUM); return nullptr;
                    }
                    fl::s24x8 cell = fl::s24x8::from_raw(cellRaw);
                    auto alias = static_cast<WorleyAliasing>(aliasByte);
                    return tag == PAT_WORLEY
                        ? worleyPattern(cell, alias)
                        : voronoiPattern(cell, alias);
                }

                default:
                    setStatusIfOk(status, DecodeStatus::UNKNOWN_TAG);
                    return nullptr;
            }
        }

        // ───── Transform decoder ───────────────────────────────────────

        // Returns true on success (transform appended to builder).
        bool decodeTransform(ByteReader &r, LayerBuilder &builder, DecodeStatus *status) {
            if (*status != DecodeStatus::OK) return false;
            uint8_t tag = r.readU8();
            if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return false; }

            switch (tag) {
                case TFM_ROTATION: {
                    uint8_t turnByte = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return false; }
                    Sf16Signal angle = decodeSignal(r, status);
                    if (*status != DecodeStatus::OK) return false;
                    builder.addTransform(RotationTransform(std::move(angle), turnByte != 0));
                    return true;
                }
                case TFM_TRANSLATION: {
                    Sf16Signal dir   = decodeSignal(r, status);
                    Sf16Signal speed = decodeSignal(r, status);
                    if (*status != DecodeStatus::OK) return false;
                    builder.addTransform(TranslationTransform(std::move(dir), std::move(speed)));
                    return true;
                }
                case TFM_ZOOM: {
                    Sf16Signal s = decodeSignal(r, status);
                    if (*status != DecodeStatus::OK) return false;
                    builder.addTransform(ZoomTransform(std::move(s)));
                    return true;
                }
                case TFM_VORTEX: {
                    Sf16Signal s = decodeSignal(r, status);
                    if (*status != DecodeStatus::OK) return false;
                    builder.addTransform(VortexTransform(std::move(s)));
                    return true;
                }
                case TFM_KALEIDOSCOPE: {
                    uint8_t facets = r.readU8();
                    uint8_t mirror = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return false; }
                    builder.addTransform(KaleidoscopeTransform(facets, mirror != 0));
                    return true;
                }
                case TFM_RADIAL_KALEIDO: {
                    uint16_t divisions = r.readU16();
                    uint8_t  mirror    = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return false; }
                    builder.addTransform(RadialKaleidoscopeTransform(divisions, mirror != 0));
                    return true;
                }
                case TFM_PALETTE: {
                    Sf16Signal offset = decodeSignal(r, status);
                    if (*status != DecodeStatus::OK) return false;
                    builder.addPaletteTransform(PaletteTransform(std::move(offset)));
                    return true;
                }
                case TFM_TILING: {
                    uint8_t mirrorByte = r.readU8();
                    uint8_t shapeByte  = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return false; }
                    if (shapeByte > static_cast<uint8_t>(TilingMaths::TileShape::HEXAGON)) {
                        setStatusIfOk(status, DecodeStatus::BAD_ENUM); return false;
                    }
                    Sf16Signal cellSize = decodeSignal(r, status);
                    if (*status != DecodeStatus::OK) return false;
                    builder.addTransform(TilingTransform(std::move(cellSize), mirrorByte != 0,
                        static_cast<TilingMaths::TileShape>(shapeByte)));
                    return true;
                }
                case TFM_FLOW_FIELD: {
                    Sf16Signal phaseSpeed   = decodeSignal(r, status);
                    Sf16Signal flowStrength = decodeSignal(r, status);
                    Sf16Signal fieldScale   = decodeSignal(r, status);
                    Sf16Signal maxOffset    = decodeSignal(r, status);
                    if (*status != DecodeStatus::OK) return false;
                    // Use the constructor's default ranges for fieldScaleRange/maxOffsetRange.
                    builder.addTransform(FlowFieldTransform(
                        std::move(phaseSpeed), std::move(flowStrength),
                        std::move(fieldScale), std::move(maxOffset)));
                    return true;
                }
                default:
                    setStatusIfOk(status, DecodeStatus::UNKNOWN_TAG);
                    return false;
            }
        }

    } // namespace (anonymous)

    std::unique_ptr<Scene> decodeScene(const uint8_t *bytes,
                                       std::size_t len,
                                       DecodeStatus *statusOut) {
        DecodeStatus localStatus = DecodeStatus::OK;
        DecodeStatus *status = &localStatus;

        ByteReader r(bytes, len);

        // Header: magic
        if (r.readU8() != 'P' || r.readU8() != 'S' ||
            r.readU8() != 'C' || r.readU8() != 0) {
            setStatusIfOk(status, r.ok() ? DecodeStatus::BAD_MAGIC : DecodeStatus::TRUNCATED);
            if (statusOut) *statusOut = *status;
            return nullptr;
        }

        // Version
        uint8_t version = r.readU8();
        if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); if (statusOut) *statusOut = *status; return nullptr; }
        if (version != 0) {
            setStatusIfOk(status, DecodeStatus::BAD_VERSION);
            if (statusOut) *statusOut = *status;
            return nullptr;
        }

        // Palette
        uint8_t paletteId = r.readU8();
        if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); if (statusOut) *statusOut = *status; return nullptr; }
        const ::CRGBPalette16 *palette = paletteById(paletteId);
        if (!palette) {
            setStatusIfOk(status, DecodeStatus::BAD_ENUM);
            if (statusOut) *statusOut = *status;
            return nullptr;
        }

        // Pattern
        std::unique_ptr<UVPattern> pattern = decodePattern(r, status);
        if (*status != DecodeStatus::OK || !pattern) {
            if (statusOut) *statusOut = *status;
            return nullptr;
        }

        LayerBuilder builder(std::move(pattern), *palette, "composer");

        // Transforms
        uint8_t transformCount = r.readU8();
        if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); if (statusOut) *statusOut = *status; return nullptr; }
        for (uint8_t i = 0; i < transformCount; ++i) {
            if (!decodeTransform(r, builder, status)) {
                if (statusOut) *statusOut = *status;
                return nullptr;
            }
        }

        // Build the scene with default (infinite) duration so SceneManager
        // never falls back to its provider after a replaceScene call.
        fl::vector<std::shared_ptr<Layer>> layers;
        layers.push_back(std::make_shared<Layer>(builder.build()));

        if (statusOut) *statusOut = DecodeStatus::OK;
        return std::make_unique<Scene>(std::move(layers));
    }
}
