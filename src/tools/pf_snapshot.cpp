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
 *
 * This tool is compiled ONLY by [env:native_pf_snapshot]; every other env
 * excludes tools/pf_snapshot.cpp so no second main() is ever linked.
 * Run:
 *   pio run -e native_pf_snapshot && .pio/build/native_pf_snapshot/program
 *
 * NOTE: 0511 (RowSegments) and 0624 (RadialPulse) are idiomatic approximations
 * of the source effects.
 */

#include "native/Arduino.h"
#include "native/FastLED.h"

#include "renderer/pipeline/patterns/patternflow/PatternFlow.h"

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
        {"pfLattice",        []{ return pfLattice(); }},
        {"pfMoire",          []{ return pfMoire(); }},
        {"pfChladni",        []{ return pfChladni(); }},
        {"pfChirp",          []{ return pfChirp(); }},
        {"pfSpiralArms",     []{ return pfSpiralArms(); }},
        {"pfRippleTank",     []{ return pfRippleTank(); }},
    };
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

// Dump one bare pattern as a grayscale PGM over a Cartesian UV grid.
void dumpPatternPgm(const PatternEntry &e) {
    auto pattern = e.make();
    auto ctx = std::make_shared<PipelineContext>();
    for (int fnum = 1; fnum <= kFrames; ++fnum) {
        pattern->advanceFrame(u0x16(0), static_cast<TimeMillis>(fnum) * kStepMs);
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

    std::printf("done: %zu patterns\n", patterns().size());
    return 0;
}
