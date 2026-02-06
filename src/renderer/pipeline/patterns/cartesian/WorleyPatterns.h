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

namespace PolarShader {
    /**
     * @brief Shared Worley/Voronoi base implementation with fixed 3x3 neighbor search.
     *
     * The cell size defines the lattice spacing in Q24.8 units and is clamped to
     * at least WorleyCellUnit (one unit). Distances are computed as squared
     * distances (no sqrt) and normalized to the PatternNormU16 domain.
     */
    class WorleyBasePattern : public CartesianPattern, public UVPattern {
    protected:
        struct Distances {
            uint64_t f1;
            uint64_t f2;
            uint32_t id;
        };

        int32_t cell_size_raw;
        uint8_t dist_shift;
        uint16_t max_dist_scaled;
        WorleyAliasing aliasing;

        explicit WorleyBasePattern(WorleyAliasing aliasingMode);

        void configureCellSize(CartQ24_8 cellSize);

        CartQ24_8 aliasingOffset() const;

        Distances computeDistances(CartQ24_8 x, CartQ24_8 y) const;

        PatternNormU16 normalizeDistance(uint64_t dist) const;

        PatternNormU16 softenValue(PatternNormU16 value) const;
    };

    /**
     * @brief Outputs the nearest-point distance (F1) field.
     *
     * @param cellSize Controls the lattice spacing. Smaller values increase cell density
     *                 and frequency; larger values produce bigger cells.
     * @param aliasingMode None = single sample, Fast = smoothstep softening, Precise = 2x2 average sampling.
     */
    class WorleyPattern : public WorleyBasePattern {
        struct Functor;
        struct UVFunctor;

    public:
        explicit WorleyPattern(
            CartQ24_8 cellSize = CartQ24_8(WorleyCellUnit),
            WorleyAliasing aliasingMode = WorleyAliasing::Fast
        );

        CartesianLayer layer(const std::shared_ptr<PipelineContext> &context) const override;

        UVLayer layer(const std::shared_ptr<PipelineContext> &context) const override;

    private:
        PatternNormU16 sampleFast(CartQ24_8 x, CartQ24_8 y) const;

        PatternNormU16 samplePrecise(CartQ24_8 x, CartQ24_8 y) const;
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
        struct Functor;
        struct UVFunctor;

    public:
        explicit VoronoiPattern(
            CartQ24_8 cellSize = CartQ24_8(WorleyCellUnit),
            WorleyAliasing aliasingMode = WorleyAliasing::Fast
        );

        CartesianLayer layer(const std::shared_ptr<PipelineContext> &context) const override;

        UVLayer layer(const std::shared_ptr<PipelineContext> &context) const override;

    private:
        PatternNormU16 sampleIdFast(CartQ24_8 x, CartQ24_8 y) const;

        PatternNormU16 sampleFastestId(
            CartQ24_8 x0, CartQ24_8 y0,
            CartQ24_8 x1, CartQ24_8 y1
        ) const;

        PatternNormU16 samplePrecise(CartQ24_8 x, CartQ24_8 y) const;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_CARTESIAN_WORLEYPATTERNS_H
