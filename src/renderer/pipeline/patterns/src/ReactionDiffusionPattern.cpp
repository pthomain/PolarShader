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
#include <FastLED.h>
#else
#include "native/FastLED.h"
#endif
#include <algorithm>
#include "renderer/pipeline/patterns/ReactionDiffusionPattern.h"

namespace PolarShader {
    namespace {
        // Diffusion rates: Du=0.2, Dv=0.1 in Q16.
        constexpr int32_t RD_DU = 13107;
        constexpr int32_t RD_DV = 6554;

        struct PresetParams { uint16_t f; uint16_t k; };

        // f and k converted to Q16 (multiply by 65536).
        constexpr PresetParams kPresets[] = {
            {1638, 3801},  // Spots:   f=0.025, k=0.058
            {2621, 3932},  // Stripes: f=0.040, k=0.060
            {3604, 4063},  // Coral:   f=0.055, k=0.062
            {1180, 3604},  // Worms:   f=0.018, k=0.055
        };
        constexpr uint16_t RD_SATURATION_DELTA = 0x0400;
        constexpr uint32_t RD_RESEED_FRAME_LIMIT = 120;

        bool needsReseed(const State &s) {
            const uint16_t * v = s.v.get();
            const uint32_t n = (uint32_t)s.width * s.height;
            uint16_t min_v = 65535;
            uint16_t max_v = 0;
            for (uint32_t i = 0; i < n; ++i) {
                const uint16_t value = v[i];
                if (value < min_v) min_v = value;
                if (value > max_v) max_v = value;
                if (max_v - min_v >= RD_SATURATION_DELTA) {
                    return false;
                }
            }
            return true;
        }
    }

    struct ReactionDiffusionPattern::RDFunctor {
        const State* state;

        PatternNormU16 operator()(UV uv) const {
            const int32_t u_raw = raw(uv.u);
            const int32_t v_raw = raw(uv.v);

            // Wrap sr16 coordinates into [0, 65536).
            int32_t uw = u_raw % 65536;
            if (uw < 0) uw += 65536;
            int32_t vw = v_raw % 65536;
            if (vw < 0) vw += 65536;

            const uint16_t width = state->width;
            const uint16_t height = state->height;

            // Scale to grid position in Q16: integer part = cell index.
            const uint32_t gx = (uint32_t)uw * width;
            const uint32_t gy = (uint32_t)vw * height;

            const uint8_t x0 = (uint8_t)(gx >> 16);
            const uint8_t y0 = (uint8_t)(gy >> 16);
            const uint8_t x1 = (x0 + 1 < width)  ? x0 + 1 : 0;
            const uint8_t y1 = (y0 + 1 < height) ? y0 + 1 : 0;

            const uint32_t fx = gx & 0xFFFFu;
            const uint32_t fy = gy & 0xFFFFu;

            const uint16_t* v = state->v.get();
            const uint32_t v00 = v[(uint16_t)y0 * width + x0];
            const uint32_t v10 = v[(uint16_t)y0 * width + x1];
            const uint32_t v01 = v[(uint16_t)y1 * width + x0];
            const uint32_t v11 = v[(uint16_t)y1 * width + x1];

            const uint32_t top = (uint32_t)(
                ((uint64_t)v00 * (65536u - fx) + (uint64_t)v10 * fx) >> 16
            );
            const uint32_t bot = (uint32_t)(
                ((uint64_t)v01 * (65536u - fx) + (uint64_t)v11 * fx) >> 16
            );
            const uint32_t res = (uint32_t)(
                ((uint64_t)top * (65536u - fy) + (uint64_t)bot * fy) >> 16
            );
            return PatternNormU16((uint16_t)res);
        }
    };

    void ReactionDiffusionPattern::seed(State& s) {
        const uint16_t n = (uint16_t)s.width * s.height;
        std::fill_n(s.u.get(), n, static_cast<uint16_t>(65535));
        std::fill_n(s.v.get(), n, static_cast<uint16_t>(0));
        std::fill_n(s.u_next.get(), n, static_cast<uint16_t>(65535));
        std::fill_n(s.v_next.get(), n, static_cast<uint16_t>(0));

        const uint8_t spread = 2 + (random8() & 0x3);
        const uint8_t cx = static_cast<uint8_t>(random16(s.width));
        const uint8_t cy = static_cast<uint8_t>(random16(s.height));
        auto wrapCoord = [](int32_t value, uint8_t dim) -> uint8_t {
            if (dim == 0) return 0;
            int32_t mod = value % dim;
            if (mod < 0) mod += dim;
            return static_cast<uint8_t>(mod);
        };
        for (int8_t dy = -static_cast<int8_t>(spread); dy <= static_cast<int8_t>(spread); ++dy) {
            for (int8_t dx = -static_cast<int8_t>(spread); dx <= static_cast<int8_t>(spread); ++dx) {
                const uint8_t y = wrapCoord(static_cast<int32_t>(cy) + dy, s.height);
                const uint8_t x = wrapCoord(static_cast<int32_t>(cx) + dx, s.width);
                const uint16_t idx = (uint16_t)y * s.width + x;
                s.u[idx] = 32768;
                s.v[idx] = static_cast<uint16_t>(16384 + (random8() & 0x3F));
            }
        }

        s.framesSinceSeed = 0;
    }

    void ReactionDiffusionPattern::step(State& s) {
        const uint8_t W = s.width;
        const uint8_t H = s.height;
        const uint32_t fk = s.f + s.k;
        const uint16_t* U = s.u.get();
        const uint16_t* V = s.v.get();
        uint16_t* Un = s.u_next.get();
        uint16_t* Vn = s.v_next.get();

        for (uint8_t y = 0; y < H; ++y) {
            for (uint8_t x = 0; x < W; ++x) {
                const uint16_t idx = (uint16_t)y * W + x;
                const uint16_t il  = (uint16_t)(x == 0 ? W - 1 : x - 1) + (uint16_t)y * W;
                const uint16_t ir  = (uint16_t)(x == W - 1 ? 0 : x + 1) + (uint16_t)y * W;
                const uint16_t iu  = (uint16_t)x + (uint16_t)(y == 0 ? H - 1 : y - 1) * W;
                const uint16_t id  = (uint16_t)x + (uint16_t)(y == H - 1 ? 0 : y + 1) * W;

                const int32_t u = U[idx];
                const int32_t v = V[idx];

                // 4-neighbour Laplacian with toroidal wrap.
                const int32_t lap_u = (int32_t)U[il] + U[ir] + U[iu] + U[id] - 4 * u;
                const int32_t lap_v = (int32_t)V[il] + V[ir] + V[iu] + V[id] - 4 * v;

                // Diffusion: Du * lap_u, Dv * lap_v (Q16 multiply, int64 to avoid overflow).
                const int32_t diff_u = (int32_t)(((int64_t)s.du * lap_u) >> 16);
                const int32_t diff_v = (int32_t)(((int64_t)s.dv * lap_v) >> 16);

                // Reaction: U * V^2 in Q16.
                const int32_t uv  = (int32_t)(((uint64_t)(uint32_t)u * (uint32_t)v)  >> 16);
                const int32_t uv2 = (int32_t)(((uint64_t)(uint32_t)uv * (uint32_t)v) >> 16);

                // Feed and kill terms.
                const int32_t f_term = (int32_t)((s.f * (uint32_t)(65536 - u)) >> 16);
                const int32_t fk_v   = (int32_t)((fk  * (uint32_t)v)           >> 16);

                // Euler step and clamp to [0, 65535].
                int32_t new_u = u + diff_u - uv2 + f_term;
                int32_t new_v = v + diff_v + uv2 - fk_v;

                if (new_u < 0) new_u = 0; else if (new_u > 65535) new_u = 65535;
                if (new_v < 0) new_v = 0; else if (new_v > 65535) new_v = 65535;

                Un[idx] = (uint16_t)new_u;
                Vn[idx] = (uint16_t)new_v;
            }
        }

        s.u.swap(s.u_next);
        s.v.swap(s.v_next);
    }

    ReactionDiffusionPattern::ReactionDiffusionPattern(
        Preset preset,
        uint8_t width,
        uint8_t height,
        uint8_t stepsPerFrame
    ) : state(std::make_shared<State>()),
        stepsPerFrame(stepsPerFrame) {
        const auto& p = kPresets[static_cast<uint8_t>(preset)];
        const uint16_t n = (uint16_t)width * height;

        state->width  = width;
        state->height = height;
        state->du     = RD_DU;
        state->dv     = RD_DV;
        state->f      = p.f;
        state->k      = p.k;
        state->u      = std::make_unique<uint16_t[]>(n);
        state->v      = std::make_unique<uint16_t[]>(n);
        state->u_next = std::make_unique<uint16_t[]>(n);
        state->v_next = std::make_unique<uint16_t[]>(n);
        state->framesSinceSeed = 0;

        seed(*state);
    }

    void ReactionDiffusionPattern::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void)progress;
        (void)elapsedMs;
        for (uint8_t i = 0; i < stepsPerFrame; ++i) {
            step(*state);
        }
        state->framesSinceSeed += stepsPerFrame;
        if (state->framesSinceSeed >= RD_RESEED_FRAME_LIMIT || needsReseed(*state)) {
            seed(*state);
        }
    }

    UVMap ReactionDiffusionPattern::layer(const std::shared_ptr<PipelineContext>& context) const {
        (void)context;
        return RDFunctor{state.get()};
    }
}
