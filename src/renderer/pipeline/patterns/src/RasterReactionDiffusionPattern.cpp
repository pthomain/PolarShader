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

#if defined(ARDUINO) || defined(__EMSCRIPTEN__)
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif

#include "renderer/pipeline/patterns/RasterReactionDiffusionPattern.h"
#include <new>

namespace PolarShader {
    namespace {
        // Diffusion rates: Du=0.2, Dv=0.1 in Q16.
        constexpr int32_t RRD_DU = 13107;
        constexpr int32_t RRD_DV = 6554;

        struct RrdPresetParams {
            uint16_t f;
            uint16_t k;
        };

        // f and k converted to Q16 (multiply by 65536).
        constexpr RrdPresetParams kRrdPresets[] = {
            {1638, 3801}, // Spots:   f=0.025, k=0.058
            {2621, 3932}, // Stripes: f=0.040, k=0.060
            {3604, 4063}, // Coral:   f=0.055, k=0.062
            {1180, 3604}, // Worms:   f=0.018, k=0.055
        };
        constexpr uint8_t kRrdPresetCount = 4u;

        constexpr uint16_t RRD_SATURATION_DELTA = 0x1000;
        constexpr uint8_t RRD_SATURATION_STREAK = 10u;

        constexpr uint16_t kMinIterations = 1u;
        constexpr uint16_t kMaxIterations = 16u;

        uint16_t clampIterations(uint16_t value) {
            if (value < kMinIterations) return kMinIterations;
            if (value > kMaxIterations) return kMaxIterations;
            return value;
        }
    }

    RasterReactionDiffusionPattern::RasterReactionDiffusionPattern(
        uint8_t preset,
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint16_t iterationsPerStep
    ) : RasterAutomaton(stepIntervalMs, seed),
        iterationsPerStep(clampIterations(iterationsPerStep)) {
        const uint8_t index = preset < kRrdPresetCount ? preset : 0u;
        f = kRrdPresets[index].f;
        k = kRrdPresets[index].k;
    }

    bool RasterReactionDiffusionPattern::allocate(uint16_t, uint16_t, uint32_t cellCount) const {
        std::unique_ptr<uint16_t[]> newU(new (std::nothrow) uint16_t[cellCount]);
        std::unique_ptr<uint16_t[]> newV(new (std::nothrow) uint16_t[cellCount]);
        std::unique_ptr<uint16_t[]> newUNext(new (std::nothrow) uint16_t[cellCount]);
        std::unique_ptr<uint16_t[]> newVNext(new (std::nothrow) uint16_t[cellCount]);
        if (!newU || !newV || !newUNext || !newVNext) return false;

        u = std::move(newU);
        v = std::move(newV);
        uNext = std::move(newUNext);
        vNext = std::move(newVNext);
        return true;
    }

    void RasterReactionDiffusionPattern::release() const {
        u.reset();
        v.reset();
        uNext.reset();
        vNext.reset();
    }

    void RasterReactionDiffusionPattern::seed(uint32_t generationSeed) const {
        if (!u || !v || !uNext || !vNext) return;

        rng = generationSeed == 0 ? 0xA5A5A5A5u : generationSeed;
        const uint32_t count = cellCount();
        for (uint32_t i = 0; i < count; ++i) {
            u[i] = 65535u;
            v[i] = 0u;
            uNext[i] = 65535u;
            vNext[i] = 0u;
        }

        // Seed a small random square patch of catalyst V.
        const uint16_t spread = static_cast<uint16_t>(2u + ((raster::lcgNext(rng) >> 16) & 0x3u));
        const uint16_t cx = static_cast<uint16_t>((raster::lcgNext(rng) >> 16) % width());
        const uint16_t cy = static_cast<uint16_t>((raster::lcgNext(rng) >> 16) % height());
        for (int32_t dy = -static_cast<int32_t>(spread); dy <= static_cast<int32_t>(spread); ++dy) {
            for (int32_t dx = -static_cast<int32_t>(spread); dx <= static_cast<int32_t>(spread); ++dx) {
                int32_t xm = (static_cast<int32_t>(cx) + dx) % width();
                if (xm < 0) xm += width();
                int32_t ym = (static_cast<int32_t>(cy) + dy) % height();
                if (ym < 0) ym += height();
                const uint32_t idx = static_cast<uint32_t>(ym) * width() + xm;
                u[idx] = 32768u;
                v[idx] = static_cast<uint16_t>(16384u + ((raster::lcgNext(rng) >> 16) & 0x3Fu));
            }
        }

        saturatedStreak = 0u;
    }

    void RasterReactionDiffusionPattern::euler() const {
        const uint16_t W = width();
        const uint16_t H = height();
        const uint32_t fk = static_cast<uint32_t>(f) + k;
        const uint16_t *U = u.get();
        const uint16_t *V = v.get();
        uint16_t *Un = uNext.get();
        uint16_t *Vn = vNext.get();

        for (uint16_t y = 0; y < H; ++y) {
            for (uint16_t x = 0; x < W; ++x) {
                const uint32_t idx = static_cast<uint32_t>(y) * W + x;
                const uint32_t il = static_cast<uint32_t>(x == 0 ? W - 1 : x - 1) + static_cast<uint32_t>(y) * W;
                const uint32_t ir = static_cast<uint32_t>(x == W - 1 ? 0 : x + 1) + static_cast<uint32_t>(y) * W;
                const uint32_t iu = static_cast<uint32_t>(x) + static_cast<uint32_t>(y == 0 ? H - 1 : y - 1) * W;
                const uint32_t id = static_cast<uint32_t>(x) + static_cast<uint32_t>(y == H - 1 ? 0 : y + 1) * W;

                const int32_t uu = U[idx];
                const int32_t vv = V[idx];

                // 4-neighbour Laplacian with toroidal wrap.
                const int32_t lap_u = static_cast<int32_t>(U[il]) + U[ir] + U[iu] + U[id] - 4 * uu;
                const int32_t lap_v = static_cast<int32_t>(V[il]) + V[ir] + V[iu] + V[id] - 4 * vv;

                // Diffusion: Du * lap_u, Dv * lap_v (Q16 multiply).
                const int32_t diff_u = static_cast<int32_t>((static_cast<int64_t>(RRD_DU) * lap_u) >> 16);
                const int32_t diff_v = static_cast<int32_t>((static_cast<int64_t>(RRD_DV) * lap_v) >> 16);

                // Reaction: U * V^2 in Q16.
                const int32_t uv = static_cast<int32_t>((static_cast<uint64_t>(static_cast<uint32_t>(uu)) * static_cast<uint32_t>(vv)) >> 16);
                const int32_t uv2 = static_cast<int32_t>((static_cast<uint64_t>(static_cast<uint32_t>(uv)) * static_cast<uint32_t>(vv)) >> 16);

                // Feed and kill terms.
                const int32_t f_term = static_cast<int32_t>((static_cast<uint32_t>(f) * static_cast<uint32_t>(65536 - uu)) >> 16);
                const int32_t fk_v = static_cast<int32_t>((fk * static_cast<uint32_t>(vv)) >> 16);

                // Euler step and clamp to [0, 65535].
                int32_t new_u = uu + diff_u - uv2 + f_term;
                int32_t new_v = vv + diff_v + uv2 - fk_v;

                if (new_u < 0) new_u = 0;
                else if (new_u > 65535) new_u = 65535;
                if (new_v < 0) new_v = 0;
                else if (new_v > 65535) new_v = 65535;

                Un[idx] = static_cast<uint16_t>(new_u);
                Vn[idx] = static_cast<uint16_t>(new_v);
            }
        }

        u.swap(uNext);
        v.swap(vNext);
    }

    bool RasterReactionDiffusionPattern::step() const {
        if (!u || !v || width() == 0 || height() == 0) return false;

        for (uint16_t i = 0; i < iterationsPerStep; ++i) {
            euler();
        }

        // Detect a saturated (flat) field: reseed once it has persisted.
        const uint16_t *V = v.get();
        const uint32_t count = cellCount();
        uint16_t minV = 65535u;
        uint16_t maxV = 0u;
        bool active = false;
        for (uint32_t i = 0; i < count; ++i) {
            const uint16_t value = V[i];
            if (value < minV) minV = value;
            if (value > maxV) maxV = value;
            if (maxV - minV >= RRD_SATURATION_DELTA) {
                active = true;
                break;
            }
        }

        if (!active) {
            if (++saturatedStreak >= RRD_SATURATION_STREAK) {
                saturatedStreak = 0u;
                return false; // Trigger a deterministic reseed.
            }
        } else {
            saturatedStreak = 0u;
        }
        return true;
    }

    RasterMap RasterReactionDiffusionPattern::rasterLayer(const std::shared_ptr<PipelineContext> &context) const {
        configure(context);
        return [this](const RasterPoint &point) {
            if (!ready() || !point.valid || !v) {
                return PatternNormU16(0);
            }
            if (point.width != width() || point.height != height() ||
                point.x >= width() || point.y >= height()) {
                return PatternNormU16(0);
            }

            const uint32_t idx = static_cast<uint32_t>(point.y) * width() + point.x;
            return PatternNormU16(v[idx]);
        };
    }
}
