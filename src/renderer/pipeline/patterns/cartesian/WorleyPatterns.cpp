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

#include "WorleyPatterns.h"
#include "renderer/pipeline/maths/PatternMaths.h"

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

    WorleyBasePattern::WorleyBasePattern(WorleyAliasing aliasingMode)
        : cell_size_raw(WorleyCellUnit),
          dist_shift(0),
          max_dist_scaled(FRACT_Q0_16_MAX),
          aliasing(aliasingMode) {
    }

    void WorleyBasePattern::configureCellSize(CartQ24_8 cellSize) {
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
    }

    CartQ24_8 WorleyBasePattern::aliasingOffset() const {
        int32_t offset = cell_size_raw >> 3;
        if (offset < 1) offset = 1;
        return CartQ24_8(offset);
    }

    WorleyBasePattern::Distances WorleyBasePattern::computeDistances(CartQ24_8 x, CartQ24_8 y) const {
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

    PatternNormU16 WorleyBasePattern::normalizeDistance(uint64_t dist) const {
        uint64_t scaled = dist >> dist_shift;
        if (scaled > max_dist_scaled) scaled = max_dist_scaled;
        return patternNormalize(static_cast<uint16_t>(scaled), 0, max_dist_scaled);
    }

    PatternNormU16 WorleyBasePattern::softenValue(PatternNormU16 value) const {
        return patternSmoothstepU16(0, FRACT_Q0_16_MAX, raw(value));
    }

    struct WorleyPattern::Functor {
        const WorleyPattern *self;

        PatternNormU16 operator()(CartQ24_8 x, CartQ24_8 y) const {
            if (self->aliasing == WorleyAliasing::Precise) {
                return self->samplePrecise(x, y);
            }
            PatternNormU16 value = self->sampleFast(x, y);
            if (self->aliasing == WorleyAliasing::Fast) {
                return self->softenValue(value);
            }
            return value;
        }
    };

    WorleyPattern::WorleyPattern(CartQ24_8 cellSize, WorleyAliasing aliasingMode)
        : WorleyBasePattern(aliasingMode) {
        configureCellSize(cellSize);
    }

    CartesianLayer WorleyPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        return Functor{this};
    }

    PatternNormU16 WorleyPattern::sampleFast(CartQ24_8 x, CartQ24_8 y) const {
        auto d = computeDistances(x, y);
        return normalizeDistance(d.f1);
    }

    PatternNormU16 WorleyPattern::samplePrecise(CartQ24_8 x, CartQ24_8 y) const {
        CartQ24_8 offset = aliasingOffset();
        uint32_t sum = 0;
        sum += raw(sampleFast(CartQ24_8(raw(x) - raw(offset)), CartQ24_8(raw(y) - raw(offset))));
        sum += raw(sampleFast(CartQ24_8(raw(x) + raw(offset)), CartQ24_8(raw(y) - raw(offset))));
        sum += raw(sampleFast(CartQ24_8(raw(x) - raw(offset)), CartQ24_8(raw(y) + raw(offset))));
        sum += raw(sampleFast(CartQ24_8(raw(x) + raw(offset)), CartQ24_8(raw(y) + raw(offset))));
        return PatternNormU16(static_cast<uint16_t>(sum >> 2));
    }

    struct VoronoiPattern::Functor {
        const VoronoiPattern *self;

        PatternNormU16 operator()(CartQ24_8 x, CartQ24_8 y) const {
            if (self->aliasing == WorleyAliasing::Precise) {
                return self->samplePrecise(x, y);
            }
            return self->sampleIdFast(x, y);
        }
    };

    VoronoiPattern::VoronoiPattern(CartQ24_8 cellSize, WorleyAliasing aliasingMode)
        : WorleyBasePattern(aliasingMode) {
        configureCellSize(cellSize);
    }

    CartesianLayer VoronoiPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        return Functor{this};
    }

    PatternNormU16 VoronoiPattern::sampleIdFast(CartQ24_8 x, CartQ24_8 y) const {
        auto d = computeDistances(x, y);
        return PatternNormU16(static_cast<uint16_t>(d.id & 0xFFFFu));
    }

    PatternNormU16 VoronoiPattern::sampleFastestId(
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

    PatternNormU16 VoronoiPattern::samplePrecise(CartQ24_8 x, CartQ24_8 y) const {
        CartQ24_8 offset = aliasingOffset();
        return sampleFastestId(
            CartQ24_8(raw(x) - raw(offset)), CartQ24_8(raw(y) - raw(offset)),
            CartQ24_8(raw(x) + raw(offset)), CartQ24_8(raw(y) + raw(offset))
        );
    }
}
