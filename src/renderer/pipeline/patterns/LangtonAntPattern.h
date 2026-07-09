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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_LANGTONANTPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_LANGTONANTPATTERN_H

#include "renderer/pipeline/patterns/base/RasterAutomaton.h"
#include <memory>

namespace PolarShader {
    // Langton's ant on a toroidal binary grid. Each ant sits on a cell facing
    // one of four directions and, every step, turns right on a "white" cell
    // (state 1) or left on a "black" cell (state 0), flips the cell it leaves,
    // then advances one cell (wrapping at the edges). The trail these simple
    // rules paint self-organises into the classic "highway". Scalar output:
    // painted cells read full intensity, blank cells read zero.
    class LangtonAntPattern : public RasterAutomaton {
    public:
        explicit LangtonAntPattern(
            uint16_t stepIntervalMs = 30,
            uint16_t seed = 0,
            uint16_t antCount = 1
        );

        RasterMap rasterLayer(const std::shared_ptr<PipelineContext>& context) const override;

    protected:
        bool allocate(uint16_t width, uint16_t height, uint32_t cellCount) const override;
        void release() const override;
        void seed(uint32_t generationSeed) const override;
        bool step() const override;

    private:
        struct Ant {
            uint16_t x;
            uint16_t y;
            uint8_t dir; // 0=up, 1=right, 2=down, 3=left
        };

        uint16_t antCount;

        mutable std::unique_ptr<uint8_t[]> cells;
        mutable std::unique_ptr<Ant[]> ants;
        mutable uint32_t rng{0};
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_LANGTONANTPATTERN_H
