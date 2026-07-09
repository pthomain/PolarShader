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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_ELEMENTARYCAPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_ELEMENTARYCAPATTERN_H

#include "renderer/pipeline/patterns/base/RasterAutomaton.h"
#include <memory>

namespace PolarShader {
    // One-dimensional (elementary / Wolfram) cellular automaton. Each step
    // derives a fresh bottom row from the current bottom row using an 8-bit
    // rule number (e.g. 30, 90, 110) applied to each cell's left/self/right
    // triple, then scrolls the whole grid up one row so history flows upward.
    class ElementaryCAPattern : public RasterAutomaton {
    public:
        explicit ElementaryCAPattern(
            uint16_t stepIntervalMs = 90,
            uint16_t seed = 0,
            uint8_t rule = 30
        );

        RasterMap rasterLayer(const std::shared_ptr<PipelineContext>& context) const override;

    protected:
        bool allocate(uint16_t width, uint16_t height, uint32_t cellCount) const override;
        void release() const override;
        void seed(uint32_t generationSeed) const override;
        bool step() const override;

    private:
        uint8_t rule;

        mutable std::unique_ptr<uint8_t[]> cells;
        mutable std::unique_ptr<uint8_t[]> rowBuf;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_ELEMENTARYCAPATTERN_H
