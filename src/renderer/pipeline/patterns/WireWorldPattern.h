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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_WIREWORLDPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_WIREWORLDPATTERN_H

#include "renderer/pipeline/patterns/base/RasterAutomaton.h"
#include <memory>

namespace PolarShader {
    // Wireworld cellular automaton. Four states: empty, electron head,
    // electron tail, conductor.
    //  - empty stays empty;
    //  - a head becomes a tail;
    //  - a tail becomes a conductor;
    //  - a conductor becomes a head iff exactly one or two of its 8 Moore
    //    neighbours are heads, otherwise it stays a conductor.
    // The board is seeded with a random conductor mesh sprinkled with a few
    // electron heads. Colour-emitting: conductor reads dim orange, tail red,
    // head bright yellow-white.
    class WireWorldPattern : public RasterAutomaton {
    public:
        explicit WireWorldPattern(
            uint16_t stepIntervalMs = 100,
            uint16_t seed = 0,
            uint16_t densityPermille = 500
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
        uint16_t densityPermille;

        mutable std::unique_ptr<uint8_t[]> cells;
        mutable std::unique_ptr<uint8_t[]> next;
        mutable uint32_t rng{0};
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_WIREWORLDPATTERN_H
