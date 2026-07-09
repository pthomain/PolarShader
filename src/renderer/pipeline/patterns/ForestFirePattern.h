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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_FORESTFIREPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_FORESTFIREPATTERN_H

#include "renderer/pipeline/patterns/base/RasterAutomaton.h"
#include <memory>

namespace PolarShader {
    // Drossel-Schwabl forest-fire model. Three states: empty, tree, burning.
    //  - a burning cell becomes empty next step;
    //  - a tree ignites if any of its 8 Moore neighbours is burning, or
    //    spontaneously ("lightning") with probability lightningPermille;
    //  - an empty cell grows a tree with probability growthPermille.
    // Colour-emitting: trees read green, fire reads red/orange.
    class ForestFirePattern : public RasterAutomaton {
    public:
        explicit ForestFirePattern(
            uint16_t stepIntervalMs = 120,
            uint16_t seed = 0,
            uint16_t growthPermille = 40,
            uint16_t lightningPermille = 2
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
        uint16_t growthPermille;
        uint16_t lightningPermille;

        mutable std::unique_ptr<uint8_t[]> cells;
        mutable std::unique_ptr<uint8_t[]> next;
        mutable uint32_t rng{0};
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_FORESTFIREPATTERN_H
