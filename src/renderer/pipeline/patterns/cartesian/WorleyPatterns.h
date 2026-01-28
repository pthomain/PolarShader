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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_CARTESIAN_WORLEYPATTERNS_H
#define POLAR_SHADER_PIPELINE_PATTERNS_CARTESIAN_WORLEYPATTERNS_H

#include "renderer/pipeline/patterns/BasePattern.h"
#include "renderer/pipeline/patterns/cartesian/WorleyConstants.h"
#include "renderer/pipeline/ranges/PatternRange.h"

namespace PolarShader {
    namespace {
        uint32_t hash32(uint32_t x) {
            x ^= x >> 16;
            x *= 0x7feb352d;
            x ^= x >> 15;
            x *= 0x846ca68b;
            x ^= x >> 16;
            return x;
        }

        uint32_t hash2(int32_t x, int32_t y) {
            uint32_t hx = hash32(static_cast<uint32_t>(x));
            uint32_t hy = hash32(static_cast<uint32_t>(y) ^ 0x9e3779b9u);
            return hx ^ (hy + 0x9e3779b9u + (hx << 6) + (hx >> 2));
        }

        int32_t floor_div(int32_t a, int32_t b) {
            int32_t q = a / b;
            int32_t r = a % b;
            if (r != 0 && ((r > 0) != (b > 0))) {
                q -= 1;
            }
            return q;
        }
    }

    /**
     * @brief Shared Worley/Voronoi base implementation with fixed 3x3 neighbor search.
     *
     * The cell size defines the lattice spacing in Q24.8 units and is clamped to
     * at least WorleyCellUnit (one unit). Distances are computed as squared
     * distances (no sqrt) and normalized to the PatternNormU16 domain.
     */
    class WorleyBasePattern : public CartesianPattern {
    protected:
        struct Distances {
            uint64_t f1;
            uint64_t f2;
            uint32_t id;
        };

        int32_t cell_size_raw;
        uint8_t dist_shift;
        uint16_t max_dist_scaled;
        PatternRange range;
        WorleyAliasing aliasing;

        explicit WorleyBasePattern(OutputSemantic semantic, WorleyAliasing aliasingMode)
            : CartesianPattern(PatternKind::Cellular, semantic),
              cell_size_raw(WorleyCellUnit),
              dist_shift(0),
              max_dist_scaled(FRACT_Q0_16_MAX),
              range(0, FRACT_Q0_16_MAX),
              aliasing(aliasingMode) {
        }

        void configureCellSize(CartQ24_8 cellSize) {
            int32_t raw_size = raw(cellSize);
            if (raw_size < WorleyCellUnit) {
                raw_size = WorleyCellUnit;
            }
            cell_size_raw = raw_size;

            uint64_t max_dist = static_cast<uint64_t>(cell_size_raw) * cell_size_raw * 2u;
            uint8_t shift = 0;
            while (max_dist > 0xFFFFu) {
                max_dist >>= 1;
                shift++;
            }
            dist_shift = shift;
            max_dist_scaled = static_cast<uint16_t>(max_dist);
            range = PatternRange(0, max_dist_scaled);
        }

        CartQ24_8 aliasingOffset() const {
            int32_t offset = cell_size_raw >> 3;
            if (offset < 1) offset = 1;
            return CartQ24_8(offset);
        }

        template<typename T>
        PatternNormU16 samplePrecise(
            CartQ24_8 x,
            CartQ24_8 y,
            const T *self,
            PatternNormU16 (T::*sampleFunc)(CartQ24_8, CartQ24_8) const
        ) const {
            CartQ24_8 offset = aliasingOffset();
            uint32_t sum = 0;
            sum += raw((self->*sampleFunc)(CartQ24_8(raw(x) - raw(offset)), CartQ24_8(raw(y) - raw(offset))));
            sum += raw((self->*sampleFunc)(CartQ24_8(raw(x) + raw(offset)), CartQ24_8(raw(y) - raw(offset))));
            sum += raw((self->*sampleFunc)(CartQ24_8(raw(x) - raw(offset)), CartQ24_8(raw(y) + raw(offset))));
            sum += raw((self->*sampleFunc)(CartQ24_8(raw(x) + raw(offset)), CartQ24_8(raw(y) + raw(offset))));
            return PatternNormU16(static_cast<uint16_t>(sum >> 2));
        }

        Distances computeDistances(CartQ24_8 x, CartQ24_8 y) const {
            int32_t x_raw = raw(x);
            int32_t y_raw = raw(y);

            int32_t cell_x = floor_div(x_raw, cell_size_raw);
            int32_t cell_y = floor_div(y_raw, cell_size_raw);

            uint64_t best1 = UINT64_MAX;
            uint64_t best2 = UINT64_MAX;
            uint32_t best_id = 0;

            for (int32_t oy = -1; oy <= 1; oy++) {
                for (int32_t ox = -1; ox <= 1; ox++) {
                    int32_t nx = cell_x + ox;
                    int32_t ny = cell_y + oy;

                    uint32_t h1 = hash2(nx, ny);
                    uint32_t h2 = hash2(nx + 1297, ny - 937);

                    uint16_t jx = static_cast<uint16_t>(h1 & 0xFFFFu);
                    uint16_t jy = static_cast<uint16_t>(h2 & 0xFFFFu);

                    int64_t jitter_x = (static_cast<int64_t>(jx) * static_cast<int64_t>(cell_size_raw)) >> 16;
                    int64_t jitter_y = (static_cast<int64_t>(jy) * static_cast<int64_t>(cell_size_raw)) >> 16;

                    int64_t origin_x = static_cast<int64_t>(nx) * cell_size_raw;
                    int64_t origin_y = static_cast<int64_t>(ny) * cell_size_raw;

                    int64_t px = origin_x + jitter_x;
                    int64_t py = origin_y + jitter_y;

                    int64_t dx = static_cast<int64_t>(x_raw) - px;
                    int64_t dy = static_cast<int64_t>(y_raw) - py;

                    uint64_t dist = static_cast<uint64_t>(dx * dx + dy * dy);

                    if (dist < best1) {
                        best2 = best1;
                        best1 = dist;
                        best_id = h1;
                    } else if (dist < best2) {
                        best2 = dist;
                    }
                }
            }

            return {best1, best2, best_id};
        }

        PatternNormU16 normalizeDistance(uint64_t dist) const {
            uint64_t scaled = dist >> dist_shift;
            if (scaled > max_dist_scaled) scaled = max_dist_scaled;
            return range.normalize(static_cast<uint16_t>(scaled));
        }

        PatternNormU16 softenValue(PatternNormU16 value) const {
            return PatternRange::smoothstep_u16(0, FRACT_Q0_16_MAX, raw(value));
        }
    };

    /**
     * @brief Outputs the nearest-point distance (F1) field.
     *
     * @param cellSize Controls the lattice spacing. Smaller values increase cell density
     *                 and frequency; larger values produce bigger cells.
     * @param aliasingMode None = single sample, Fast = smoothstep softening, Precise = 2x2 average sampling.
     */
    class WorleyPattern : public WorleyBasePattern {
        struct Functor {
            const WorleyPattern *self;

            PatternNormU16 operator()(CartQ24_8 x, CartQ24_8 y) const {
                if (self->aliasing == WorleyAliasing::Precise) {
                    return self->samplePrecise(x, y, self, &WorleyPattern::sampleFast);
                }
                PatternNormU16 value = self->sampleFast(x, y);
                if (self->aliasing == WorleyAliasing::Fast) {
                    return self->softenValue(value);
                }
                return value;
            }
        };

    public:
        explicit WorleyPattern(
            CartQ24_8 cellSize = CartQ24_8(WorleyCellUnit),
            WorleyAliasing aliasingMode = WorleyAliasing::Fast
        ) : WorleyBasePattern(OutputSemantic::Field, aliasingMode) {
            configureCellSize(cellSize);
        }

        CartesianLayer layer() const override {
            return Functor{this};
        }

    private:
        PatternNormU16 sampleFast(CartQ24_8 x, CartQ24_8 y) const {
            auto d = computeDistances(x, y);
            return normalizeDistance(d.f1);
        }
    };

    /**
     * @brief Outputs a stable, hash-based cell ID mapped to 0..65535.
     *
     * The ID is derived from the cell hash (not normalized). Use this for categorical
     * cell selection or palette indexing.
     *
     * @param cellSize Controls the lattice spacing. Smaller values increase cell density;
     *                 larger values produce fewer, bigger cells.
     * @param aliasingMode None = single sample, Fast = no-op for IDs, Precise = 2x2 average sampling (uses nearest among samples).
     */
    class VoronoiPattern : public WorleyBasePattern {
        struct Functor {
            const VoronoiPattern *self;

            PatternNormU16 operator()(CartQ24_8 x, CartQ24_8 y) const {
                if (self->aliasing == WorleyAliasing::Precise) {
                    CartQ24_8 offset = self->aliasingOffset();
                    return self->sampleFastestId(
                        CartQ24_8(raw(x) - raw(offset)), CartQ24_8(raw(y) - raw(offset)),
                        CartQ24_8(raw(x) + raw(offset)), CartQ24_8(raw(y) + raw(offset))
                    );
                }
                return self->sampleIdFast(x, y);
            }
        };

    public:
        explicit VoronoiPattern(
            CartQ24_8 cellSize = CartQ24_8(WorleyCellUnit),
            WorleyAliasing aliasingMode = WorleyAliasing::Fast
        ) : WorleyBasePattern(OutputSemantic::Id, aliasingMode) {
            configureCellSize(cellSize);
        }

        CartesianLayer layer() const override {
            return Functor{this};
        }

    private:
        PatternNormU16 sampleIdFast(CartQ24_8 x, CartQ24_8 y) const {
            auto d = computeDistances(x, y);
            return PatternNormU16(static_cast<uint16_t>(d.id & 0xFFFFu));
        }

        PatternNormU16 sampleFastestId(
            CartQ24_8 x0, CartQ24_8 y0,
            CartQ24_8 x1, CartQ24_8 y1
        ) const {
            Distances best = computeDistances(x0, y0);
            Distances d1 = computeDistances(x1, y0);
            if (d1.f1 < best.f1) best = d1;
            Distances d2 = computeDistances(x0, y1);
            if (d2.f1 < best.f1) best = d2;
            Distances d3 = computeDistances(x1, y1);
            if (d3.f1 < best.f1) best = d3;

            return PatternNormU16(static_cast<uint16_t>(best.id & 0xFFFFu));
        }
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_CARTESIAN_WORLEYPATTERNS_H
