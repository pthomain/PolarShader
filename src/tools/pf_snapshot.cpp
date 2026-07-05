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

/*
 * PatternFlow snapshot dumper (native desktop tool, own main()).
 *
 * Writes visual proofs of the ported PatternFlow effects into
 * build/pf_snapshots/:
 *   - PGM per bare pf* pattern  : sample layer()(UV) over a Cartesian UV grid.
 *   - PPM per pf*Preset builder : sample the built Layer::compile() ColourMap
 *     over a polar display disc (angle, radius) -> CRGB, which is the only
 *     view that exercises the transform + palette stack (kaleidoscope, vortex,
 *     tiling, rotation, zoom, palette-offset).
 *
 * This tool is compiled ONLY by [env:native_pf_snapshot]; every other env
 * excludes tools/pf_snapshot.cpp so no second main() is ever linked.
 * Run:
 *   pio run -e native_pf_snapshot && .pio/build/native_pf_snapshot/program
 *
 * NOTE: 0511 (RowSegments) and 0624 (RadialPulse) are idiomatic approximations
 * of the source effects; 0526 (KaleidoVortex) is a transform recipe over Ripple.
 */

#include "native/Arduino.h"
#include "native/FastLED.h"

#include "renderer/pipeline/patterns/patternflow/PatternFlow.h"
#include "renderer/pipeline/patterns/patternflow/PatternFlowPresets.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

using namespace PolarShader;

namespace {

constexpr int kGrid = 160;        // PGM / PPM edge in pixels
constexpr int kFrames = 8;        // advanceFrame steps before sampling
constexpr TimeMillis kStepMs = 200u;

const char *kOutDir = "build/pf_snapshots";

// Bare pf* pattern factories, paired with their file-stem names.
struct PatternEntry {
    const char *name;
    std::function<std::unique_ptr<UVPattern>()> make;
};

std::vector<PatternEntry> patterns() {
    return {
        {"pfConcentricGrid", []{ return pfConcentricGrid(); }},
        {"pfDualAxis",       []{ return pfDualAxis(); }},
        {"pfRowSegments",    []{ return pfRowSegments(); }}, // 0511 (approx)
        {"pfPetals",         []{ return pfPetals(); }},
        {"pfShapes",         []{ return pfShapes(); }},
        {"pfCounterRibbons", []{ return pfCounterRibbons(); }},
        {"pfQuadDirectional",[]{ return pfQuadDirectional(); }},
        {"pfOrganic",        []{ return pfOrganic(); }},
        {"pfTendrils",       []{ return pfTendrils(); }},
        {"pfRipple",         []{ return pfRipple(); }},        // 0524-2
        {"pfDots",           []{ return pfDots(); }},
        {"pfTopographic",    []{ return pfTopographic(); }},   // 0529
        {"pfLiquidGate",     []{ return pfLiquidGate(); }},    // 0530
        {"pfPosterized",     []{ return pfPosterized(); }},
        {"pfWaveMatrix",     []{ return pfWaveMatrix(); }},
        {"pfPlasma",         []{ return pfPlasma(); }},
        {"pfCross",          []{ return pfCross(); }},         // 0614-2
        {"pfRadialPulse",    []{ return pfRadialPulse(); }},   // 0624 (approx)
    };
}

// Full-saturation hue wheel -> RGB (native stub has no CHSV).
CRGB hueToRgb(double h) { // h in [0, 1)
    double x = h * 6.0;
    int seg = static_cast<int>(x) % 6;
    double f = x - std::floor(x);
    uint8_t up = static_cast<uint8_t>(f * 255.0);
    uint8_t dn = static_cast<uint8_t>((1.0 - f) * 255.0);
    switch (seg) {
        case 0:  return CRGB(255, up, 0);
        case 1:  return CRGB(dn, 255, 0);
        case 2:  return CRGB(0, 255, up);
        case 3:  return CRGB(0, dn, 255);
        case 4:  return CRGB(up, 0, 255);
        default: return CRGB(255, 0, dn);
    }
}

// Rainbow spread: deterministic, distinct across the wheel -- so the PPM shows
// field structure rather than a palette artefact.
CRGBPalette16 snapshotPalette() {
    CRGBPalette16 p;
    for (int i = 0; i < 16; ++i) {
        p.entries[i] = hueToRgb(static_cast<double>(i) / 16.0);
    }
    return p;
}

// Map a pixel column/row in [0, kGrid) to a UV raw coord in the renderer's
// normalised [0, 1] domain (Q16.16), matching what patterns receive live.
int32_t pixelToUvRaw(int px) {
    double n = (kGrid > 1) ? (static_cast<double>(px) / (kGrid - 1)) : 0.5;
    return static_cast<int32_t>(n * 65535.0);
}

bool writePgm(const std::string &path, const std::vector<uint8_t> &gray) {
    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    std::fprintf(f, "P5\n%d %d\n255\n", kGrid, kGrid);
    std::fwrite(gray.data(), 1, gray.size(), f);
    std::fclose(f);
    return true;
}

bool writePpm(const std::string &path, const std::vector<uint8_t> &rgb) {
    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    std::fprintf(f, "P6\n%d %d\n255\n", kGrid, kGrid);
    std::fwrite(rgb.data(), 1, rgb.size(), f);
    std::fclose(f);
    return true;
}

// Dump one bare pattern as a grayscale PGM over a Cartesian UV grid.
void dumpPatternPgm(const PatternEntry &e) {
    auto pattern = e.make();
    auto ctx = std::make_shared<PipelineContext>();
    for (int fnum = 1; fnum <= kFrames; ++fnum) {
        pattern->advanceFrame(f16(0), static_cast<TimeMillis>(fnum) * kStepMs);
    }
    UVMap map = pattern->layer(ctx);

    std::vector<uint8_t> gray(static_cast<size_t>(kGrid) * kGrid);
    for (int y = 0; y < kGrid; ++y) {
        int32_t vRaw = pixelToUvRaw(y);
        for (int x = 0; x < kGrid; ++x) {
            UV uv(fl::s16x16::from_raw(pixelToUvRaw(x)), fl::s16x16::from_raw(vRaw));
            gray[static_cast<size_t>(y) * kGrid + x] =
                static_cast<uint8_t>(raw(map(uv)) >> 8);
        }
    }
    std::string path = std::string(kOutDir) + "/pattern_" + e.name + ".pgm";
    std::printf("  %s %s\n", writePgm(path, gray) ? "wrote" : "FAILED", path.c_str());
}

// Dump one preset as an RGB PPM over a polar display disc. The ColourMap takes
// (angle, radius) display coords and converts to UV internally, so this is the
// view that proves the transform + palette stack actually applied.
void dumpPresetPpm(const PfEffect &effect, const CRGBPalette16 &palette) {
    Layer layer = effect.preset(palette).build();
    for (int fnum = 1; fnum <= kFrames; ++fnum) {
        layer.advanceFrame(f16(0), static_cast<TimeMillis>(fnum) * kStepMs);
    }
    auto cm = layer.compile();

    std::vector<uint8_t> rgb(static_cast<size_t>(kGrid) * kGrid * 3, 0);
    const double half = (kGrid - 1) / 2.0;
    for (int y = 0; y < kGrid; ++y) {
        for (int x = 0; x < kGrid; ++x) {
            double dx = (x - half) / half; // [-1, 1]
            double dy = (y - half) / half;
            double r = std::sqrt(dx * dx + dy * dy);
            size_t idx = (static_cast<size_t>(y) * kGrid + x) * 3;
            if (r > 1.0) continue; // outside the disc stays black
            double ang = std::atan2(dy, dx);           // [-pi, pi]
            double turns = ang / (2.0 * M_PI);
            if (turns < 0.0) turns += 1.0;              // [0, 1)
            f16 angle(static_cast<uint16_t>(turns * 65535.0));
            f16 radius(static_cast<uint16_t>(r * 65535.0));
            CRGB c = (*cm)(angle, radius);
            rgb[idx + 0] = c.r;
            rgb[idx + 1] = c.g;
            rgb[idx + 2] = c.b;
        }
    }
    std::string stem = effect.variant;
    std::string path = std::string(kOutDir) + "/preset_" + stem + ".ppm";
    std::printf("  %s %s\n", writePpm(path, rgb) ? "wrote" : "FAILED", path.c_str());
}

} // namespace

int main() {
    std::error_code ec;
    std::filesystem::create_directories(kOutDir, ec);
    if (ec) {
        std::fprintf(stderr, "cannot create %s: %s\n", kOutDir, ec.message().c_str());
        return 1;
    }

    std::printf("PGM (bare patterns) -> %s\n", kOutDir);
    for (const auto &e : patterns()) {
        dumpPatternPgm(e);
    }

    std::printf("PPM (presets) -> %s\n", kOutDir);
    CRGBPalette16 palette = snapshotPalette();
    for (const auto &effect : PF_EFFECTS) {
        dumpPresetPpm(effect, palette);
    }

    std::printf("done: %zu patterns, %d presets\n", patterns().size(), 19);
    return 0;
}
