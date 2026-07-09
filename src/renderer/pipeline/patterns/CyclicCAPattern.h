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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_CYCLICCAPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_CYCLICCAPATTERN_H

#include "renderer/pipeline/patterns/base/RasterAutomaton.h"
#include <memory>

namespace PolarShader {
    // Cyclic cellular automaton (Greenberg-Hastings): a cell in state k
    // advances to (k+1) mod numStates when at least `threshold` of its Moore
    // neighbours already hold that successor state. Produces rotating spiral
    // waves; state maps directly to hue.
    class CyclicCAPattern : public RasterAutomaton {
    public:
        explicit CyclicCAPattern(
            uint16_t stepIntervalMs = 120,
            uint16_t seed = 0,
            uint8_t numStates = 8,
            uint8_t threshold = 1
        );

        bool emitsColour() const override { return true; }
        RasterMap rasterLayer(const std::shared_ptr<PipelineContext>& context) const override;
        RasterColourMap rasterColourLayer(const std::shared_ptr<PipelineContext>& context) const override;

    protected:
        bool allocate(uint16_t width, uint16_t height, uint32_t cellCount) const override;
        void release() const override;
        void seed(uint32_t generationSeed) const override;
        bool step() const override;

    private:
        uint8_t numStates;
        uint8_t threshold;

        mutable std::unique_ptr<uint8_t[]> cells;
        mutable std::unique_ptr<uint8_t[]> next;

        static uint8_t clampNumStates(uint8_t value);
        static uint8_t clampThreshold(uint8_t value);
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_CYCLICCAPATTERN_H
