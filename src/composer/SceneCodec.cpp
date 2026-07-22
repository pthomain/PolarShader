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

// PSC v1 wire-format decoder. See SceneCodec.h for the format spec.
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

            SIG_SINE              = 0x10, // body: signal phaseVelocity + i32 phaseOffset(s0x16)
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
            SIG_SCALE             = 0x20, // body: signal signal + u16 factor (u0x16)
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

            // PatternFlow bare field patterns (one tag per Variant). Body layout:
            // static config bytes (if any) THEN ordered S0x16Signal blobs.
            PAT_PF_DUAL_AXIS        = 0x0D, // signals: phaseSpeed, warp, thickness
            PAT_PF_COUNTER_RIBBONS  = 0x0E, // signals: phaseSpeed, warp, thickness
            PAT_PF_QUAD_DIRECTIONAL = 0x0F, // signals: phaseSpeed, warp, thickness
            PAT_PF_POSTERIZED       = 0x10, // u8 posterizeLevels; signals: phaseSpeed, warp, thickness
            PAT_PF_CROSS            = 0x11, // signals: phaseSpeed, warp, thickness
            PAT_PF_PETALS           = 0x12, // u8 petalCount; signals: phaseSpeed, fold, thickness
            PAT_PF_RIPPLE           = 0x13, // u8 waveCount; signals: phaseSpeed, warp, thickness
            PAT_PF_ORGANIC          = 0x14, // u8 contourLevels + u8 hardEdges; signals: phaseSpeed, tension
            PAT_PF_TOPOGRAPHIC      = 0x15, // u8 contourLevels + u8 hardEdges; signals: phaseSpeed, tension
            PAT_PF_PLASMA           = 0x16, // signals: phaseSpeed, warp, thickness
            PAT_PF_TENDRILS         = 0x17, // signals: phaseSpeed, warp, thickness
            PAT_PF_LIQUID_GATE      = 0x18, // signals: phaseSpeed, warp, thickness
            PAT_PF_CONCENTRIC_GRID  = 0x19, // u8 cellCount; signals: phaseSpeed, warp, thickness
            PAT_PF_ROW_SEGMENTS     = 0x1A, // u8 cellCount; signals: phaseSpeed, warp, thickness
            PAT_PF_SHAPES           = 0x1B, // u8 cellCount; signals: phaseSpeed, warp, thickness
            PAT_PF_DOTS             = 0x1C, // u8 cellCount; signals: phaseSpeed, warp, thickness
            PAT_PF_WAVE_MATRIX      = 0x1D, // u8 cellCount; signals: phaseSpeed, warp, thickness
            PAT_PF_RADIAL_PULSE     = 0x1E, // u8 cellCount; signals: phaseSpeed, warp, thickness
            PAT_PALETTE_GLOW        = 0x1F, // body: optional signal speed + optional signal tileScale
            PAT_ROCAILLE            = 0x20, // body: 9 signals matching source sliders
            PAT_PROTEAN_CLOUDS      = 0x21, // body: speed, warp, frequency, brightness
            PAT_XOR                 = 0x22, // u8 gridSize + u16 speed
            PAT_OCTGRAMS            = 0x23, // body: speed, travel, pulse, density, glow
            PAT_ROTATING_SQUARES    = 0x24, // body: speed, thickness, pulse, brightness
            PAT_STARRY_PLANES       = 0x25, // body: speed, planeSpacing, starSize, path, brightness
            PAT_TRIG_FIELD          = 0x26, // body: zoom, yOffset, waveScale, speed, colorSpread, brightness
            PAT_STAR_FIELD_TRAVEL   = 0x27, // body: speed, density, trail, starSize, brightness
            PAT_RASTER_CONWAY       = 0x2B, // u16 stepIntervalMs + u16 seed + u16 densityPermille
            PAT_RASTER_CYCLIC_CA    = 0x2C, // u16 stepIntervalMs + u16 seed + u8 numStates + u8 threshold
            PAT_RASTER_BRIANS_BRAIN = 0x2D, // u16 stepIntervalMs + u16 seed + u16 densityPermille
            PAT_RASTER_LIFE_VARIANT = 0x2E, // u16 stepIntervalMs + u16 seed + u16 densityPermille + u8 rule
            PAT_RASTER_ELEMENTARY   = 0x2F, // u16 stepIntervalMs + u16 seed + u8 rule
            PAT_RASTER_MATRIX_RAIN  = 0x30, // u16 stepIntervalMs + u16 seed + u8 fadeAmount
            PAT_RASTER_RIPPLE       = 0x31, // u16 stepIntervalMs + u16 seed + u8 damping
            PAT_RASTER_FOREST_FIRE  = 0x32, // u16 stepIntervalMs + u16 seed + u16 growthPermille + u16 lightningPermille
            PAT_RASTER_WIREWORLD    = 0x33, // u16 stepIntervalMs + u16 seed + u16 densityPermille
            PAT_RASTER_LANGTON_ANT  = 0x34, // u16 stepIntervalMs + u16 seed + u16 antCount
            PAT_RASTER_REACTION_DIFF = 0x35, // u8 preset + u16 stepIntervalMs + u16 seed + u16 iterationsPerStep
            PAT_PF_LATTICE          = 0x36, // signals: phaseSpeed, warp, thickness
            PAT_PF_MOIRE            = 0x37, // signals: phaseSpeed, warp, thickness
            PAT_PF_CHLADNI          = 0x38, // u8 modeCount; signals: phaseSpeed, warp, thickness
            PAT_PF_CHIRP            = 0x39, // u8 waveCount; signals: phaseSpeed, fold, thickness
            PAT_PF_SPIRAL_ARMS      = 0x3A, // u8 armCount; signals: phaseSpeed, fold, thickness
            PAT_PF_RIPPLE_TANK      = 0x3B, // u8 sourceCount; signals: phaseSpeed, warp, thickness
            PAT_NOISE_BASIC_LOOP    = 0x3C, // u16 loopPeriodMs; signal depthSpeed
        };

        enum TransformTag : uint8_t {
            TFM_ROTATION          = 0x00, // body: u8 isAngleTurn + signal angle
            TFM_TRANSLATION       = 0x01, // body: signal direction + signal speed
            TFM_ZOOM              = 0x02, // body: signal scale
            TFM_VORTEX            = 0x03, // body: signal strength
            TFM_KALEIDOSCOPE      = 0x04, // body: u8 nbFacets + u8 isMirrored
            TFM_RADIAL_KALEIDO    = 0x05, // body: u16 radialDivisions + u8 isMirrored
            TFM_TILING            = 0x07, // body: u8 mirrored + u8 shape + signal cellSize
            TFM_FLOW_FIELD        = 0x08, // body: 4 signals (phaseSpeed, flowStrength, fieldScale, maxOffset)
            TFM_PALETTE_CLIP      = 0x09, // body: u16 maxFeather + u8 tintMode(0 hue-remap/1 colour-mask/2 native) + signal offset + signal clip
        };

        // ───── Bounds-checked byte reader ─────────────────────────────

        class ByteReader {
        public:
            ByteReader(const uint8_t *data, std::size_t len) : data_(data), len_(len), pos_(0), bad_(false) {}

            std::size_t remaining() const { return bad_ ? 0 : (len_ - pos_); }
            bool ok() const { return !bad_; }
            void fail() { bad_ = true; }
            bool atEnd() const { return ok() && pos_ == len_; }

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

            const uint8_t *readBytes(std::size_t count) {
                if (remaining() < count) { bad_ = true; return nullptr; }
                const uint8_t *ptr = data_ + pos_;
                pos_ += count;
                return ptr;
            }

            ByteReader subReader(std::size_t count) {
                const uint8_t *ptr = readBytes(count);
                return ByteReader(ptr ? ptr : data_, ptr ? count : 0);
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

        constexpr uint8_t kMaxSignalDecodeDepth = 64;

        // Loop period (scene duration) threaded into noise signals so they loop
        // seamlessly in looping mode. Set at the top of decodeSceneWithDuration
        // before any signal is decoded; 0 means non-looping (live playback).
        TimeMillis g_signalLoopPeriodMs = 0;

        S0x16Signal decodeSignal(ByteReader &r, DecodeStatus *status, uint8_t version);
        S0x16Signal decodeSignalAtDepth(ByteReader &r,
                                       DecodeStatus *status,
                                       uint8_t version,
                                       uint8_t depth);

        S0x16Signal decodeSignalBody(uint8_t tag,
                                    ByteReader &r,
                                    DecodeStatus *status,
                                    uint8_t version,
                                    uint8_t depth) {
            if (*status != DecodeStatus::OK) return S0x16Signal();

            switch (tag) {
                case SIG_CONSTANT: {
                    uint16_t permille = r.readU16();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return S0x16Signal(); }
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
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return S0x16Signal(); }
                    if (loopByte > static_cast<uint8_t>(LoopMode::SATURATE)) {
                        setStatusIfOk(status, DecodeStatus::BAD_ENUM); return S0x16Signal();
                    }
                    LoopMode loop = static_cast<LoopMode>(loopByte);
                    switch (tag) {
                        case SIG_LINEAR:           return linear(duration, loop);
                        case SIG_QUADRATIC_IN:     return quadraticIn(duration, loop);
                        case SIG_QUADRATIC_OUT:    return quadraticOut(duration, loop);
                        case SIG_QUADRATIC_IN_OUT: return quadraticInOut(duration, loop);
                    }
                    return S0x16Signal();
                }

                case SIG_SINE:
                case SIG_TRIANGLE:
                case SIG_SQUARE:
                case SIG_SAWTOOTH:
                case SIG_NOISE: {
                    S0x16Signal pv = decodeSignalAtDepth(r, status, version, depth + 1);
                    int32_t phaseRaw = r.readI32();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return S0x16Signal(); }
                    if (*status != DecodeStatus::OK) return S0x16Signal();
                    s0x16 phase = s0x16(phaseRaw);
                    switch (tag) {
                        case SIG_SINE:     return sine(std::move(pv), phase);
                        case SIG_TRIANGLE: return triangle(std::move(pv), phase);
                        case SIG_SQUARE:   return square(std::move(pv), phase);
                        case SIG_SAWTOOTH: return sawtooth(std::move(pv), phase);
                        case SIG_NOISE:    return noise(std::move(pv), phase, g_signalLoopPeriodMs);
                    }
                    return S0x16Signal();
                }

                case SIG_SINE_BOUNDED:
                case SIG_TRIANGLE_BOUNDED:
                case SIG_SQUARE_BOUNDED:
                case SIG_SAWTOOTH_BOUNDED:
                case SIG_NOISE_BOUNDED: {
                    S0x16Signal pv      = decodeSignalAtDepth(r, status, version, depth + 1);
                    S0x16Signal floorS  = decodeSignalAtDepth(r, status, version, depth + 1);
                    S0x16Signal ceilS   = decodeSignalAtDepth(r, status, version, depth + 1);
                    if (*status != DecodeStatus::OK) return S0x16Signal();
                    switch (tag) {
                        case SIG_SINE_BOUNDED:     return sine(std::move(pv), std::move(floorS), std::move(ceilS));
                        case SIG_TRIANGLE_BOUNDED: return triangle(std::move(pv), std::move(floorS), std::move(ceilS));
                        case SIG_SQUARE_BOUNDED:   return square(std::move(pv), std::move(floorS), std::move(ceilS));
                        case SIG_SAWTOOTH_BOUNDED: return sawtooth(std::move(pv), std::move(floorS), std::move(ceilS));
                        case SIG_NOISE_BOUNDED:    return noise(std::move(pv), std::move(floorS), std::move(ceilS), g_signalLoopPeriodMs);
                    }
                    return S0x16Signal();
                }

                case SIG_SINE_BOUNDED_PH:
                case SIG_TRIANGLE_BOUNDED_PH:
                case SIG_SQUARE_BOUNDED_PH:
                case SIG_SAWTOOTH_BOUNDED_PH:
                case SIG_NOISE_BOUNDED_PH: {
                    S0x16Signal pv = decodeSignalAtDepth(r, status, version, depth + 1);
                    int32_t phaseRaw = r.readI32();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return S0x16Signal(); }
                    S0x16Signal floorS = decodeSignalAtDepth(r, status, version, depth + 1);
                    S0x16Signal ceilS  = decodeSignalAtDepth(r, status, version, depth + 1);
                    if (*status != DecodeStatus::OK) return S0x16Signal();
                    s0x16 phase = s0x16(phaseRaw);
                    switch (tag) {
                        case SIG_SINE_BOUNDED_PH:     return sine(std::move(pv), phase, std::move(floorS), std::move(ceilS));
                        case SIG_TRIANGLE_BOUNDED_PH: return triangle(std::move(pv), phase, std::move(floorS), std::move(ceilS));
                        case SIG_SQUARE_BOUNDED_PH:   return square(std::move(pv), phase, std::move(floorS), std::move(ceilS));
                        case SIG_SAWTOOTH_BOUNDED_PH: return sawtooth(std::move(pv), phase, std::move(floorS), std::move(ceilS));
                        case SIG_NOISE_BOUNDED_PH:    return noise(std::move(pv), phase, std::move(floorS), std::move(ceilS), g_signalLoopPeriodMs);
                    }
                    return S0x16Signal();
                }

                case SIG_SMAP: {
                    S0x16Signal s     = decodeSignalAtDepth(r, status, version, depth + 1);
                    S0x16Signal floor = decodeSignalAtDepth(r, status, version, depth + 1);
                    S0x16Signal ceil  = decodeSignalAtDepth(r, status, version, depth + 1);
                    if (*status != DecodeStatus::OK) return S0x16Signal();
                    return smap(std::move(s), std::move(floor), std::move(ceil));
                }

                case SIG_SCALE: {
                    S0x16Signal s = decodeSignalAtDepth(r, status, version, depth + 1);
                    uint16_t factorRaw = r.readU16();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return S0x16Signal(); }
                    if (*status != DecodeStatus::OK) return S0x16Signal();
                    return scale(std::move(s), u0x16(factorRaw));
                }

                default:
                    setStatusIfOk(status, DecodeStatus::UNKNOWN_TAG);
                    return S0x16Signal();
            }
        }

        S0x16Signal decodeSignalAtDepth(ByteReader &r,
                                       DecodeStatus *status,
                                       uint8_t version,
                                       uint8_t depth) {
            if (*status != DecodeStatus::OK) return S0x16Signal();
            if (depth > kMaxSignalDecodeDepth) {
                setStatusIfOk(status, DecodeStatus::BAD_ENUM);
                return S0x16Signal();
            }
            uint8_t tag = r.readU8();
            if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return S0x16Signal(); }

            uint16_t bodyLen = r.readU16();
            if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return S0x16Signal(); }
            ByteReader body = r.subReader(bodyLen);
            if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return S0x16Signal(); }
            S0x16Signal signal = decodeSignalBody(tag, body, status, version, depth);
            if (*status == DecodeStatus::OK && (!body.ok() || !body.atEnd())) {
                setStatusIfOk(status, body.ok() ? DecodeStatus::BAD_ENUM : DecodeStatus::TRUNCATED);
                return S0x16Signal();
            }
            return signal;
        }

        S0x16Signal decodeSignal(ByteReader &r, DecodeStatus *status, uint8_t version) {
            return decodeSignalAtDepth(r, status, version, 0);
        }

        // ───── Pattern decoder ─────────────────────────────────────────

        std::unique_ptr<UVPattern> decodePatternBody(uint8_t tag, ByteReader &r, DecodeStatus *status, uint8_t version) {
            if (*status != DecodeStatus::OK) return nullptr;

            switch (tag) {
                case PAT_NOISE_BASIC: {
                    S0x16Signal depth = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return noisePattern(std::move(depth));
                }
                case PAT_NOISE_BASIC_LOOP: {
                    uint16_t loopPeriodMs = r.readU16();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    if (loopPeriodMs == 0) { setStatusIfOk(status, DecodeStatus::BAD_VALUE); return nullptr; }
                    S0x16Signal depth = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return noiseLoopPattern(std::move(depth), loopPeriodMs);
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
                    S0x16Signal xDrift   = decodeSignal(r, status, version);
                    S0x16Signal yDrift   = decodeSignal(r, status, version);
                    S0x16Signal amp      = decodeSignal(r, status, version);
                    S0x16Signal freq     = decodeSignal(r, status, version);
                    S0x16Signal endSpeed = decodeSignal(r, status, version);
                    S0x16Signal halfLife = decodeSignal(r, status, version);
                    S0x16Signal orbSpeed = decodeSignal(r, status, version);
                    S0x16Signal orbRad   = decodeSignal(r, status, version);
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
                    S0x16Signal radial   = decodeSignal(r, status, version);
                    S0x16Signal angular  = decodeSignal(r, status, version);
                    S0x16Signal halfLife = decodeSignal(r, status, version);
                    S0x16Signal emitter  = decodeSignal(r, status, version);
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
                    S0x16Signal tightness = decodeSignal(r, status, version);
                    S0x16Signal armThick  = decodeSignal(r, status, version);
                    S0x16Signal rotSpeed  = decodeSignal(r, status, version);
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
                    S0x16Signal xDrift   = decodeSignal(r, status, version);
                    S0x16Signal yDrift   = decodeSignal(r, status, version);
                    S0x16Signal amp      = decodeSignal(r, status, version);
                    S0x16Signal freq     = decodeSignal(r, status, version);
                    S0x16Signal endSpeed = decodeSignal(r, status, version);
                    S0x16Signal fade     = decodeSignal(r, status, version);
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

                // ── PatternFlow bare fields ──────────────────────────
                // Interference/Plasma variants: no config, 3 signals.
                case PAT_PF_DUAL_AXIS:
                case PAT_PF_COUNTER_RIBBONS:
                case PAT_PF_QUAD_DIRECTIONAL:
                case PAT_PF_CROSS:
                case PAT_PF_PLASMA:
                case PAT_PF_TENDRILS:
                case PAT_PF_LIQUID_GATE:
                case PAT_PF_LATTICE:
                case PAT_PF_MOIRE: {
                    S0x16Signal phaseSpeed = decodeSignal(r, status, version);
                    S0x16Signal warp       = decodeSignal(r, status, version);
                    S0x16Signal thickness  = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    switch (tag) {
                        case PAT_PF_DUAL_AXIS:        return pfDualAxis(std::move(phaseSpeed), std::move(warp), std::move(thickness));
                        case PAT_PF_COUNTER_RIBBONS:  return pfCounterRibbons(std::move(phaseSpeed), std::move(warp), std::move(thickness));
                        case PAT_PF_QUAD_DIRECTIONAL: return pfQuadDirectional(std::move(phaseSpeed), std::move(warp), std::move(thickness));
                        case PAT_PF_CROSS:            return pfCross(std::move(phaseSpeed), std::move(warp), std::move(thickness));
                        case PAT_PF_PLASMA:           return pfPlasma(std::move(phaseSpeed), std::move(warp), std::move(thickness));
                        case PAT_PF_TENDRILS:         return pfTendrils(std::move(phaseSpeed), std::move(warp), std::move(thickness));
                        case PAT_PF_LIQUID_GATE:      return pfLiquidGate(std::move(phaseSpeed), std::move(warp), std::move(thickness));
                        case PAT_PF_LATTICE:          return pfLattice(std::move(phaseSpeed), std::move(warp), std::move(thickness));
                        case PAT_PF_MOIRE:            return pfMoire(std::move(phaseSpeed), std::move(warp), std::move(thickness));
                    }
                    return nullptr;
                }

                case PAT_PF_POSTERIZED: {
                    uint8_t levels = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    S0x16Signal phaseSpeed = decodeSignal(r, status, version);
                    S0x16Signal warp       = decodeSignal(r, status, version);
                    S0x16Signal thickness  = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return pfPosterized(levels, std::move(phaseSpeed), std::move(warp), std::move(thickness));
                }

                case PAT_PF_CHLADNI: {
                    uint8_t modeCount = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    S0x16Signal phaseSpeed = decodeSignal(r, status, version);
                    S0x16Signal warp       = decodeSignal(r, status, version);
                    S0x16Signal thickness  = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return pfChladni(modeCount, std::move(phaseSpeed), std::move(warp), std::move(thickness));
                }

                case PAT_PF_CHIRP: {
                    uint8_t waveCount = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    S0x16Signal phaseSpeed = decodeSignal(r, status, version);
                    S0x16Signal fold       = decodeSignal(r, status, version);
                    S0x16Signal thickness  = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return pfChirp(waveCount, std::move(phaseSpeed), std::move(fold), std::move(thickness));
                }

                case PAT_PF_SPIRAL_ARMS: {
                    uint8_t armCount = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    S0x16Signal phaseSpeed = decodeSignal(r, status, version);
                    S0x16Signal fold       = decodeSignal(r, status, version);
                    S0x16Signal thickness  = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return pfSpiralArms(armCount, std::move(phaseSpeed), std::move(fold), std::move(thickness));
                }

                case PAT_PF_RIPPLE_TANK: {
                    uint8_t sourceCount = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    S0x16Signal phaseSpeed = decodeSignal(r, status, version);
                    S0x16Signal warp       = decodeSignal(r, status, version);
                    S0x16Signal thickness  = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return pfRippleTank(sourceCount, std::move(phaseSpeed), std::move(warp), std::move(thickness));
                }

                case PAT_PF_PETALS: {
                    uint8_t petalCount = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    S0x16Signal phaseSpeed = decodeSignal(r, status, version);
                    S0x16Signal fold       = decodeSignal(r, status, version);
                    S0x16Signal thickness  = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return pfPetals(petalCount, std::move(phaseSpeed), std::move(fold), std::move(thickness));
                }

                case PAT_PF_RIPPLE: {
                    uint8_t waveCount = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    S0x16Signal phaseSpeed = decodeSignal(r, status, version);
                    S0x16Signal warp       = decodeSignal(r, status, version);
                    S0x16Signal thickness  = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return pfRipple(waveCount, std::move(phaseSpeed), std::move(warp), std::move(thickness));
                }

                // Contour variants: u8 contourLevels + u8 hardEdges, 2 signals.
                case PAT_PF_ORGANIC:
                case PAT_PF_TOPOGRAPHIC: {
                    uint8_t contourLevels = r.readU8();
                    uint8_t hardEdges     = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    S0x16Signal phaseSpeed = decodeSignal(r, status, version);
                    S0x16Signal tension    = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return tag == PAT_PF_ORGANIC
                        ? pfOrganic(contourLevels, hardEdges != 0, std::move(phaseSpeed), std::move(tension))
                        : pfTopographic(contourLevels, hardEdges != 0, std::move(phaseSpeed), std::move(tension));
                }

                // Cellular variants: u8 cellCount, 3 signals.
                case PAT_PF_CONCENTRIC_GRID:
                case PAT_PF_ROW_SEGMENTS:
                case PAT_PF_SHAPES:
                case PAT_PF_DOTS:
                case PAT_PF_WAVE_MATRIX:
                case PAT_PF_RADIAL_PULSE: {
                    uint8_t cellCount = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    S0x16Signal phaseSpeed = decodeSignal(r, status, version);
                    S0x16Signal warp       = decodeSignal(r, status, version);
                    S0x16Signal thickness  = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    switch (tag) {
                        case PAT_PF_CONCENTRIC_GRID: return pfConcentricGrid(cellCount, std::move(phaseSpeed), std::move(warp), std::move(thickness));
                        case PAT_PF_ROW_SEGMENTS:    return pfRowSegments(cellCount, std::move(phaseSpeed), std::move(warp), std::move(thickness));
                        case PAT_PF_SHAPES:          return pfShapes(cellCount, std::move(phaseSpeed), std::move(warp), std::move(thickness));
                        case PAT_PF_DOTS:            return pfDots(cellCount, std::move(phaseSpeed), std::move(warp), std::move(thickness));
                        case PAT_PF_WAVE_MATRIX:     return pfWaveMatrix(cellCount, std::move(phaseSpeed), std::move(warp), std::move(thickness));
                        case PAT_PF_RADIAL_PULSE:    return pfRadialPulse(cellCount, std::move(phaseSpeed), std::move(warp), std::move(thickness));
                    }
                    return nullptr;
                }

                case PAT_XOR: {
                    uint8_t gridSize = r.readU8();
                    uint16_t speed = r.readU16();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    return xorPattern(gridSize, speed);
                }

                case PAT_PALETTE_GLOW: {
                    S0x16Signal speed = constant(1000);
                    S0x16Signal tileScale = constant(500);
                    if (r.remaining() > 0) speed = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    if (r.remaining() > 0) tileScale = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return paletteGlowPattern(std::move(speed), std::move(tileScale));
                }

                case PAT_ROCAILLE: {
                    S0x16Signal scale = decodeSignal(r, status, version);
                    S0x16Signal length = decodeSignal(r, status, version);
                    S0x16Signal detail = decodeSignal(r, status, version);
                    S0x16Signal turbulence = decodeSignal(r, status, version);
                    S0x16Signal frequency = decodeSignal(r, status, version);
                    S0x16Signal speed = decodeSignal(r, status, version);
                    S0x16Signal layers = decodeSignal(r, status, version);
                    S0x16Signal hue = decodeSignal(r, status, version);
                    S0x16Signal glow = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return rocaillePattern(
                        std::move(scale),
                        std::move(length),
                        std::move(detail),
                        std::move(turbulence),
                        std::move(frequency),
                        std::move(speed),
                        std::move(layers),
                        std::move(hue),
                        std::move(glow)
                    );
                }

                case PAT_PROTEAN_CLOUDS: {
                    S0x16Signal speed = decodeSignal(r, status, version);
                    S0x16Signal warp = decodeSignal(r, status, version);
                    S0x16Signal frequency = decodeSignal(r, status, version);
                    S0x16Signal brightness = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return proteanCloudsPattern(
                        std::move(speed),
                        std::move(warp),
                        std::move(frequency),
                        std::move(brightness)
                    );
                }

                case PAT_OCTGRAMS: {
                    S0x16Signal speed = decodeSignal(r, status, version);
                    S0x16Signal travel = decodeSignal(r, status, version);
                    S0x16Signal pulse = decodeSignal(r, status, version);
                    S0x16Signal density = decodeSignal(r, status, version);
                    S0x16Signal glow = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return octgramsPattern(
                        std::move(speed),
                        std::move(travel),
                        std::move(pulse),
                        std::move(density),
                        std::move(glow)
                    );
                }

                case PAT_ROTATING_SQUARES: {
                    S0x16Signal speed = decodeSignal(r, status, version);
                    S0x16Signal thickness = decodeSignal(r, status, version);
                    S0x16Signal pulse = decodeSignal(r, status, version);
                    S0x16Signal brightness = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return rotatingSquaresPattern(
                        std::move(speed),
                        std::move(thickness),
                        std::move(pulse),
                        std::move(brightness)
                    );
                }

                case PAT_STARRY_PLANES: {
                    S0x16Signal speed = decodeSignal(r, status, version);
                    S0x16Signal planeSpacing = decodeSignal(r, status, version);
                    S0x16Signal starSize = decodeSignal(r, status, version);
                    S0x16Signal path = decodeSignal(r, status, version);
                    S0x16Signal brightness = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return starryPlanesPattern(
                        std::move(speed),
                        std::move(planeSpacing),
                        std::move(starSize),
                        std::move(path),
                        std::move(brightness)
                    );
                }

                case PAT_TRIG_FIELD: {
                    S0x16Signal zoom = decodeSignal(r, status, version);
                    S0x16Signal yOffset = decodeSignal(r, status, version);
                    S0x16Signal waveScale = decodeSignal(r, status, version);
                    S0x16Signal speed = decodeSignal(r, status, version);
                    S0x16Signal colorSpread = decodeSignal(r, status, version);
                    S0x16Signal brightness = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return trigFieldPattern(
                        std::move(zoom),
                        std::move(yOffset),
                        std::move(waveScale),
                        std::move(speed),
                        std::move(colorSpread),
                        std::move(brightness)
                    );
                }

                case PAT_STAR_FIELD_TRAVEL: {
                    S0x16Signal speed = decodeSignal(r, status, version);
                    S0x16Signal density = decodeSignal(r, status, version);
                    S0x16Signal trail = decodeSignal(r, status, version);
                    S0x16Signal starSize = decodeSignal(r, status, version);
                    S0x16Signal brightness = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return nullptr;
                    return starFieldTravelPattern(
                        std::move(speed),
                        std::move(density),
                        std::move(trail),
                        std::move(starSize),
                        std::move(brightness)
                    );
                }

                case PAT_RASTER_CONWAY: {
                    uint16_t stepIntervalMs = r.readU16();
                    uint16_t seed = r.readU16();
                    uint16_t densityPermille = r.readU16();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    return conwayPattern(stepIntervalMs, seed, densityPermille);
                }

                case PAT_RASTER_CYCLIC_CA: {
                    uint16_t stepIntervalMs = r.readU16();
                    uint16_t seed = r.readU16();
                    uint8_t numStates = r.readU8();
                    uint8_t threshold = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    return cyclicCAPattern(stepIntervalMs, seed, numStates, threshold);
                }

                case PAT_RASTER_BRIANS_BRAIN: {
                    uint16_t stepIntervalMs = r.readU16();
                    uint16_t seed = r.readU16();
                    uint16_t densityPermille = r.readU16();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    return briansBrainPattern(stepIntervalMs, seed, densityPermille);
                }

                case PAT_RASTER_LIFE_VARIANT: {
                    uint16_t stepIntervalMs = r.readU16();
                    uint16_t seed = r.readU16();
                    uint16_t densityPermille = r.readU16();
                    uint8_t rule = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    if (rule > static_cast<uint8_t>(LifeVariantPattern::Rule::Seeds)) {
                        setStatusIfOk(status, DecodeStatus::BAD_ENUM); return nullptr;
                    }
                    return lifeVariantPattern(
                        stepIntervalMs, seed, densityPermille,
                        static_cast<LifeVariantPattern::Rule>(rule));
                }

                case PAT_RASTER_ELEMENTARY: {
                    uint16_t stepIntervalMs = r.readU16();
                    uint16_t seed = r.readU16();
                    uint8_t rule = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    return elementaryCAPattern(stepIntervalMs, seed, rule);
                }

                case PAT_RASTER_MATRIX_RAIN: {
                    uint16_t stepIntervalMs = r.readU16();
                    uint16_t seed = r.readU16();
                    uint8_t fadeAmount = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    return matrixRainPattern(stepIntervalMs, seed, fadeAmount);
                }

                case PAT_RASTER_RIPPLE: {
                    uint16_t stepIntervalMs = r.readU16();
                    uint16_t seed = r.readU16();
                    uint8_t damping = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    return ripplePattern(stepIntervalMs, seed, damping);
                }

                case PAT_RASTER_FOREST_FIRE: {
                    uint16_t stepIntervalMs = r.readU16();
                    uint16_t seed = r.readU16();
                    uint16_t growthPermille = r.readU16();
                    uint16_t lightningPermille = r.readU16();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    return forestFirePattern(stepIntervalMs, seed, growthPermille, lightningPermille);
                }

                case PAT_RASTER_WIREWORLD: {
                    uint16_t stepIntervalMs = r.readU16();
                    uint16_t seed = r.readU16();
                    uint16_t densityPermille = r.readU16();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    return wireWorldPattern(stepIntervalMs, seed, densityPermille);
                }

                case PAT_RASTER_LANGTON_ANT: {
                    uint16_t stepIntervalMs = r.readU16();
                    uint16_t seed = r.readU16();
                    uint16_t antCount = r.readU16();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    return langtonAntPattern(stepIntervalMs, seed, antCount);
                }

                case PAT_RASTER_REACTION_DIFF: {
                    uint8_t preset = r.readU8();
                    uint16_t stepIntervalMs = r.readU16();
                    uint16_t seed = r.readU16();
                    uint16_t iterationsPerStep = r.readU16();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
                    if (preset > static_cast<uint8_t>(RasterReactionDiffusionPattern::Preset::Worms)) {
                        setStatusIfOk(status, DecodeStatus::BAD_ENUM); return nullptr;
                    }
                    return rasterReactionDiffusionPattern(preset, stepIntervalMs, seed, iterationsPerStep);
                }

                default:
                    setStatusIfOk(status, DecodeStatus::UNKNOWN_TAG);
                    return nullptr;
            }
        }

        std::unique_ptr<UVPattern> decodePattern(ByteReader &r, DecodeStatus *status, uint8_t version) {
            if (*status != DecodeStatus::OK) return nullptr;
            uint8_t tag = r.readU8();
            if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }

            uint16_t bodyLen = r.readU16();
            if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
            ByteReader body = r.subReader(bodyLen);
            if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return nullptr; }
            std::unique_ptr<UVPattern> pattern = decodePatternBody(tag, body, status, version);
            if (*status == DecodeStatus::OK && (!body.ok() || !body.atEnd())) {
                setStatusIfOk(status, body.ok() ? DecodeStatus::BAD_ENUM : DecodeStatus::TRUNCATED);
                return nullptr;
            }
            return pattern;
        }

        // ───── Transform decoder ───────────────────────────────────────

        bool decodeTransformBody(uint8_t tag, ByteReader &r, LayerBuilder &builder, DecodeStatus *status, uint8_t version) {
            if (*status != DecodeStatus::OK) return false;

            switch (tag) {
                case TFM_ROTATION: {
                    uint8_t turnByte = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return false; }
                    S0x16Signal angle = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return false;
                    builder.addTransform(RotationTransform(std::move(angle), turnByte != 0));
                    return true;
                }
                case TFM_TRANSLATION: {
                    S0x16Signal dir   = decodeSignal(r, status, version);
                    S0x16Signal speed = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return false;
                    builder.addTransform(TranslationTransform(std::move(dir), std::move(speed)));
                    return true;
                }
                case TFM_ZOOM: {
                    S0x16Signal s = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return false;
                    builder.addTransform(ZoomTransform(std::move(s)));
                    return true;
                }
                case TFM_VORTEX: {
                    S0x16Signal s = decodeSignal(r, status, version);
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
                case TFM_PALETTE_CLIP: {
                    uint16_t maxFeatherRaw = r.readU16();
                    // Single byte: 0 = hue-remap, 1 = colour-mask,
                    // 2 = native (bypass palette).
                    uint8_t tintModeByte = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return false; }

                    PipelineContext::PaletteTintMode tintMode;
                    switch (tintModeByte) {
                        case 0:
                            tintMode = PipelineContext::PaletteTintMode::HueRemap;
                            break;
                        case 1:
                            tintMode = PipelineContext::PaletteTintMode::ColourMask;
                            break;
                        case 2:
                            tintMode = PipelineContext::PaletteTintMode::Native;
                            break;
                        default:
                            setStatusIfOk(status, DecodeStatus::BAD_ENUM);
                            return false;
                    }

                    S0x16Signal offset = decodeSignal(r, status, version);
                    S0x16Signal clip = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return false;
                    builder.addPaletteTransform(PaletteTransform(
                        std::move(offset), std::move(clip), u0x16(maxFeatherRaw),
                        tintMode));
                    return true;
                }
                case TFM_TILING: {
                    uint8_t mirrorByte = r.readU8();
                    uint8_t shapeByte  = r.readU8();
                    if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return false; }
                    if (shapeByte > static_cast<uint8_t>(TilingMaths::TileShape::HEXAGON)) {
                        setStatusIfOk(status, DecodeStatus::BAD_ENUM); return false;
                    }
                    S0x16Signal cellSize = decodeSignal(r, status, version);
                    if (*status != DecodeStatus::OK) return false;
                    builder.addTransform(TilingTransform(std::move(cellSize), mirrorByte != 0,
                        static_cast<TilingMaths::TileShape>(shapeByte)));
                    return true;
                }
                case TFM_FLOW_FIELD: {
                    S0x16Signal phaseSpeed   = decodeSignal(r, status, version);
                    S0x16Signal flowStrength = decodeSignal(r, status, version);
                    S0x16Signal fieldScale   = decodeSignal(r, status, version);
                    S0x16Signal maxOffset    = decodeSignal(r, status, version);
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

        // Returns true on success (transform appended to builder).
        bool decodeTransform(ByteReader &r, LayerBuilder &builder, DecodeStatus *status, bool allowUvTransforms, uint8_t version) {
            if (*status != DecodeStatus::OK) return false;
            uint8_t tag = r.readU8();
            if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return false; }

            if (!allowUvTransforms && tag != TFM_PALETTE_CLIP) {
                setStatusIfOk(status, DecodeStatus::BAD_ENUM);
                return false;
            }

            uint16_t bodyLen = r.readU16();
            if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return false; }
            ByteReader body = r.subReader(bodyLen);
            if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); return false; }
            bool ok = decodeTransformBody(tag, body, builder, status, version);
            if (*status == DecodeStatus::OK && (!body.ok() || !body.atEnd())) {
                setStatusIfOk(status, body.ok() ? DecodeStatus::BAD_ENUM : DecodeStatus::TRUNCATED);
                return false;
            }
            return ok;
        }

    } // namespace (anonymous)

    std::unique_ptr<Scene> decodeSceneWithDuration(const uint8_t *bytes,
                                                   std::size_t len,
                                                   TimeMillis durationMs,
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
        if (version != 1) {
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

        // Looping mode is signalled by a finite scene duration; propagate it to
        // noise signals so they cross-dissolve into a seamless loop. UINT32_MAX
        // (live playback via decodeScene) keeps signals free-running.
        g_signalLoopPeriodMs = (durationMs == UINT32_MAX) ? 0 : durationMs;

        // Pattern
        std::unique_ptr<UVPattern> pattern = decodePattern(r, status, version);
        if (*status != DecodeStatus::OK || !pattern) {
            if (statusOut) *statusOut = *status;
            return nullptr;
        }

        const bool allowUvTransforms = pattern->domain() != PatternDomain::RasterGrid;
        LayerBuilder builder(std::move(pattern), *palette, "composer");
        // Palette id 0 is Rainbow; a hue-remap onto it is redundant, so colour
        // patterns render their emitted hue natively instead.
        builder.setPaletteIsRainbow(paletteId == 0);

        // Transforms
        uint8_t transformCount = r.readU8();
        if (!r.ok()) { setStatusIfOk(status, DecodeStatus::TRUNCATED); if (statusOut) *statusOut = *status; return nullptr; }
        for (uint8_t i = 0; i < transformCount; ++i) {
            if (!decodeTransform(r, builder, status, allowUvTransforms, version)) {
                if (statusOut) *statusOut = *status;
                return nullptr;
            }
        }

        if (!r.atEnd()) {
            setStatusIfOk(status, DecodeStatus::BAD_ENUM);
            if (statusOut) *statusOut = *status;
            return nullptr;
        }

        fl::vector<std::shared_ptr<Layer>> layers;
        layers.push_back(std::make_shared<Layer>(builder.build()));

        if (statusOut) *statusOut = DecodeStatus::OK;
        return std::make_unique<Scene>(std::move(layers), durationMs);
    }

    std::unique_ptr<Scene> decodeScene(const uint8_t *bytes,
                                       std::size_t len,
                                       DecodeStatus *statusOut) {
        // Live composer replacement scenes do not expire; otherwise the
        // SceneManager would fall back to its provider after a replaceScene().
        return decodeSceneWithDuration(bytes, len, UINT32_MAX, statusOut);
    }
}
