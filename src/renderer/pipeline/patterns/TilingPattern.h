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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_TILINGPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_TILINGPATTERN_H

#include "renderer/pipeline/maths/TilingMaths.h"
#include "renderer/pipeline/patterns/base/UVPattern.h"
#include "renderer/pipeline/signals/SignalTypes.h"

namespace PolarShader {
    /**
     * Shape-aware tiling pattern with adjacency-safe N-coloring and hard cell edges.
     */
    class TilingPattern : public UVPattern {
    public:
        using TileShape = TilingMaths::TileShape;

    private:
        // Stateless sampler used by the pipeline.
        struct UVTilingFunctor;
        struct State {
            int32_t cell_size_raw = (1 << R8_FRAC_BITS);
        };

        Sf16Signal cell_size_signal;
        uint16_t cell_size_u16;
        uint8_t color_count;
        TileShape shape;
        State state;

    public:
        explicit TilingPattern(
            uint16_t cellSize = 32,
            uint8_t colorCount = 3,
            TileShape shape = TileShape::HEXAGON
        );

        explicit TilingPattern(
            Sf16Signal cellSize,
            uint8_t colorCount = 3,
            TileShape shape = TileShape::HEXAGON
        );

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;

        UVMap layer(const std::shared_ptr<PipelineContext> &context) const override;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_TILINGPATTERN_H
