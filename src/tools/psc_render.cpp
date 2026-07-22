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
 * .PSC → frames exporter (native desktop tool, own main()).
 *
 * Samples a composition (.psc) through the REAL C++ pipeline over a display
 * (.pds) for one loop period and dumps the raw per-LED colours so a GIF
 * assembler (scripts/render_gif.py) can render a hero animation that matches
 * hardware exactly. This env defines POLAR_SHADER_REAL_NOISE, so inoise16
 * resolves to the vendored FastLED 3.10.4 implementation
 * (src/tools/thirdparty/RealNoise16.cpp) rather than the native fake hash.
 *
 * Outputs (into <out_dir>):
 *   geometry.csv  led_index,x,y   (x,y normalised to [-1,1], written once)
 *   frames.bin    N × ledCount × 3 bytes, RGB, row-major by (frame, led)
 *   meta.json     fps_eff, period_ms, ledCount, frame_ms, frames
 *
 * The tool is display-agnostic / composition-agnostic: it uses the .pds
 * geometry and samples every LED with spec.toRenderPoint(i) (carrying raster
 * coords when present), so it works for any polar or raster composition.
 *
 * Compiled ONLY by [env:native_psc_render]; every other env excludes
 * tools/psc_render.cpp so no second main() is ever linked.
 * Run:
 *   pio run -e native_psc_render && \
 *     .pio/build/native_psc_render/program tools/gif/hero.psc \
 *       displays/fibonacci.pds build/gif/ --period-ms 10000 --fps 25 --require-loop
 */

#include "native/Arduino.h"
#include "native/FastLED.h"

#include "composer/SceneCodec.h"
#include "display/DisplaySpecCodec.h"
#include "display/LoadedDisplaySpec.h"
#include "display/WebDisplayGeometry.h"
#include "renderer/RenderPoint.h"
#include "renderer/scene/Scene.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace PolarShader;

namespace {

bool readFile(const std::string &path, std::vector<uint8_t> &out) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) return false;
    const std::streamsize size = f.tellg();
    if (size < 0) return false;
    f.seekg(0, std::ios::beg);
    out.resize(static_cast<size_t>(size));
    return static_cast<bool>(f.read(reinterpret_cast<char *>(out.data()), size));
}

// Mean absolute per-byte RGB delta between two equal-length frames.
double frameDelta(const uint8_t *a, const uint8_t *b, size_t n) {
    if (n == 0) return 0.0;
    uint64_t acc = 0;
    for (size_t i = 0; i < n; ++i) {
        acc += static_cast<uint64_t>(std::abs(static_cast<int>(a[i]) - static_cast<int>(b[i])));
    }
    return static_cast<double>(acc) / static_cast<double>(n);
}

struct Args {
    std::string pscPath;
    std::string pdsPath;
    std::string outDir;
    uint32_t periodMs = 10000;
    double fps = 25.0;
    bool requireLoop = false;
    bool valid = false;
};

Args parseArgs(int argc, char **argv) {
    Args a;
    std::vector<std::string> positional;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--period-ms" && i + 1 < argc) {
            a.periodMs = static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 10));
        } else if (arg == "--fps" && i + 1 < argc) {
            a.fps = std::strtod(argv[++i], nullptr);
        } else if (arg == "--require-loop") {
            a.requireLoop = true;
        } else {
            positional.push_back(arg);
        }
    }
    if (positional.size() >= 3) {
        a.pscPath = positional[0];
        a.pdsPath = positional[1];
        a.outDir = positional[2];
        a.valid = true;
    }
    return a;
}

} // namespace

int main(int argc, char **argv) {
    Args args = parseArgs(argc, argv);
    if (!args.valid) {
        std::fprintf(stderr,
            "usage: psc_render <composition.psc> <display.pds> <out_dir> "
            "[--period-ms 10000] [--fps 25] [--require-loop]\n");
        return 2;
    }
    if (args.periodMs == 0) {
        std::fprintf(stderr, "error: --period-ms must be > 0\n");
        return 2;
    }
    if (args.fps < 10.0) {
        std::fprintf(stderr, "error: --fps must be >= 10 (per-frame step must stay under the 200ms clamp)\n");
        return 2;
    }

    // --- Load inputs ---
    std::vector<uint8_t> pscBytes;
    if (!readFile(args.pscPath, pscBytes)) {
        std::fprintf(stderr, "error: cannot read %s\n", args.pscPath.c_str());
        return 1;
    }
    std::vector<uint8_t> pdsBytes;
    if (!readFile(args.pdsPath, pdsBytes)) {
        std::fprintf(stderr, "error: cannot read %s\n", args.pdsPath.c_str());
        return 1;
    }

    DisplaySpecDecodeStatus dstat = DisplaySpecDecodeStatus::OK;
    std::unique_ptr<LoadedDisplaySpec> spec =
        decodeDisplaySpec(pdsBytes.data(), pdsBytes.size(), &dstat);
    if (!spec) {
        std::fprintf(stderr, "error: failed to decode %s (status %u)\n",
                     args.pdsPath.c_str(), static_cast<unsigned>(dstat));
        return 1;
    }

    composer::DecodeStatus sstat = composer::DecodeStatus::OK;
    std::unique_ptr<Scene> scene = composer::decodeSceneWithDuration(
        pscBytes.data(), pscBytes.size(), static_cast<TimeMillis>(args.periodMs), &sstat);
    if (!scene) {
        std::fprintf(stderr, "error: failed to decode %s (status %u)\n",
                     args.pscPath.c_str(), static_cast<unsigned>(sstat));
        return 1;
    }

    // Raster-aware compile: populate RasterDisplayInfo when the display carries
    // a raster grid; otherwise a default (polar-only) info is fine.
    RasterDisplayInfo rasterInfo{};
    if (spec->hasRaster) {
        rasterInfo.valid = true;
        rasterInfo.width = spec->rasterWidth;
        rasterInfo.height = spec->rasterHeight;
        rasterInfo.cellCount = static_cast<uint32_t>(spec->rasterCells.size());
    }
    scene->compile(rasterInfo);

    const uint16_t ledCount = spec->nbLeds();
    if (ledCount == 0) {
        std::fprintf(stderr, "error: display has no LEDs\n");
        return 1;
    }

    // --- Frame count: round-to-nearest, then redefine effective fps so the
    // sampled field spans exactly period_ms at the endpoints. ---
    const long N = std::lround(static_cast<double>(args.periodMs) * args.fps / 1000.0);
    if (N < 2) {
        std::fprintf(stderr,
            "error: computed frame count N=%ld < 2 (period_ms=%u, fps=%.3f)\n",
            N, args.periodMs, args.fps);
        return 1;
    }
    const double fpsEff = static_cast<double>(N) * 1000.0 / static_cast<double>(args.periodMs);
    const double frameMs = static_cast<double>(args.periodMs) / static_cast<double>(N);

    // --- Geometry (write once), normalised to [-1,1] preserving aspect. ---
    WebDisplayGeometry geom = buildLoadedWebGeometry(*spec);
    if (geom.points.size() != ledCount) {
        std::fprintf(stderr, "error: geometry point count %zu != ledCount %u\n",
                     geom.points.size(), ledCount);
        return 1;
    }
    float minX = geom.points[0].x, maxX = minX;
    float minY = geom.points[0].y, maxY = minY;
    for (const auto &p : geom.points) {
        minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
        minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
    }
    const float cx = (minX + maxX) * 0.5f;
    const float cy = (minY + maxY) * 0.5f;
    float scale = std::max((maxX - minX) * 0.5f, (maxY - minY) * 0.5f);
    if (scale <= 0.0f) scale = 1.0f;

    std::error_code ec;
    std::filesystem::create_directories(args.outDir, ec);

    const std::string geomPath = args.outDir + "/geometry.csv";
    {
        std::ofstream gf(geomPath);
        if (!gf) {
            std::fprintf(stderr, "error: cannot write %s\n", geomPath.c_str());
            return 1;
        }
        gf << "led_index,x,y\n";
        for (uint16_t i = 0; i < ledCount; ++i) {
            const float nx = (geom.points[i].x - cx) / scale;
            const float ny = (geom.points[i].y - cy) / scale;
            gf << i << "," << nx << "," << ny << "\n";
        }
    }

    // --- Render N frames. ---
    const size_t frameBytes = static_cast<size_t>(ledCount) * 3u;
    std::vector<uint8_t> frames(static_cast<size_t>(N) * frameBytes);
    for (long f = 0; f < N; ++f) {
        const TimeMillis elapsedMs = static_cast<TimeMillis>(
            std::lround(static_cast<double>(f) * static_cast<double>(args.periodMs) / static_cast<double>(N)));
        const uint16_t progress = static_cast<uint16_t>(
            (static_cast<uint64_t>(elapsedMs) * 65535ull) / args.periodMs);
        scene->advanceFrame(u0x16(progress), elapsedMs);

        uint8_t *dst = frames.data() + static_cast<size_t>(f) * frameBytes;
        for (uint16_t i = 0; i < ledCount; ++i) {
            const CRGB c = scene->sample(0, spec->toRenderPoint(i));
            dst[i * 3 + 0] = c.r;
            dst[i * 3 + 1] = c.g;
            dst[i * 3 + 2] = c.b;
        }
    }

    const std::string framesPath = args.outDir + "/frames.bin";
    {
        std::ofstream ff(framesPath, std::ios::binary);
        if (!ff || !ff.write(reinterpret_cast<const char *>(frames.data()),
                             static_cast<std::streamsize>(frames.size()))) {
            std::fprintf(stderr, "error: cannot write %s\n", framesPath.c_str());
            return 1;
        }
    }

    // --- Optional seam validation (opt-in; non-looping compositions skip it). ---
    if (args.requireLoop) {
        std::vector<double> interior;
        interior.reserve(static_cast<size_t>(N - 1));
        for (long i = 0; i + 1 < N; ++i) {
            interior.push_back(frameDelta(
                frames.data() + static_cast<size_t>(i) * frameBytes,
                frames.data() + static_cast<size_t>(i + 1) * frameBytes,
                frameBytes));
        }
        std::vector<double> sorted = interior;
        std::sort(sorted.begin(), sorted.end());
        const double median = sorted.empty() ? 0.0 :
            (sorted.size() % 2 ? sorted[sorted.size() / 2]
                               : 0.5 * (sorted[sorted.size() / 2 - 1] + sorted[sorted.size() / 2]));
        const double wrap = frameDelta(
            frames.data() + static_cast<size_t>(N - 1) * frameBytes,
            frames.data(), frameBytes);
        const double kEpsilon = 2.0; // absolute floor on the 0..255 scale
        const double threshold = std::max(kEpsilon, 1.5 * median);
        std::printf("seam: wrap=%.4f median_interior=%.4f threshold=%.4f\n",
                    wrap, median, threshold);
        if (wrap > threshold) {
            std::fprintf(stderr,
                "error: --require-loop failed: wrap delta %.4f exceeds threshold %.4f "
                "(composition does not loop seamlessly over %u ms)\n",
                wrap, threshold, args.periodMs);
            return 1;
        }
    }

    // --- meta.json ---
    const std::string metaPath = args.outDir + "/meta.json";
    {
        std::ofstream mf(metaPath);
        if (!mf) {
            std::fprintf(stderr, "error: cannot write %s\n", metaPath.c_str());
            return 1;
        }
        mf << "{\n"
           << "  \"fps_eff\": " << fpsEff << ",\n"
           << "  \"period_ms\": " << args.periodMs << ",\n"
           << "  \"ledCount\": " << ledCount << ",\n"
           << "  \"frames\": " << N << ",\n"
           << "  \"frame_ms\": " << frameMs << "\n"
           << "}\n";
    }

    // --- Report. GIF delays are quantised to centiseconds (10 ms). ---
    const long delayCs = std::lround(frameMs / 10.0);
    const double gifDurationMs = static_cast<double>(N) * static_cast<double>(delayCs) * 10.0;
    const double sampledDurationMs = static_cast<double>(args.periodMs);

    std::printf("psc_render: %ld frames, %u LEDs\n", N, ledCount);
    std::printf("  requested fps=%.3f -> effective fps=%.3f (frame_ms=%.4f)\n",
                args.fps, fpsEff, frameMs);
    std::printf("  sampled duration=%.1f ms; GIF-quantized duration=%.1f ms (delay=%ld cs/frame)\n",
                sampledDurationMs, gifDurationMs, delayCs);
    if (std::abs(gifDurationMs - sampledDurationMs) > 0.5) {
        std::printf("  WARNING: GIF-quantized duration differs from sampled duration; "
                    "for exact timing use period_ms/N a whole multiple of 10 ms.\n");
    }
    std::printf("  wrote %s, %s, %s\n", geomPath.c_str(), framesPath.c_str(), metaPath.c_str());
    return 0;
}
