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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_LIFEVARIANTPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_LIFEVARIANTPATTERN_H

#include "renderer/pipeline/patterns/base/RasterAutomaton.h"
#include <memory>

namespace PolarShader {
    // Life-like cellular automata that share Conway's Moore-neighbour count but
    // use different birth/survival rules. A dead cell is born when its live
    // neighbour count is in the birth set; a live cell survives when its count
    // is in the survival set.
    class LifeVariantPattern : public RasterAutomaton {
    public:
        enum class Rule : uint8_t {
            HighLife = 0,   // B36/S23
            DayAndNight = 1, // B3678/S34678
            Seeds = 2        // B2/S (no survival)
        };

        explicit LifeVariantPattern(
            uint16_t stepIntervalMs = 200,
            uint16_t seed = 0,
            uint16_t densityPermille = 350,
            Rule rule = Rule::HighLife
        );

        RasterMap rasterLayer(const std::shared_ptr<PipelineContext>& context) const override;

    protected:
        bool allocate(uint16_t width, uint16_t height, uint32_t cellCount) const override;
        void release() const override;
        void seed(uint32_t generationSeed) const override;
        bool step() const override;

    private:
        uint16_t densityPermille;
        uint16_t birthMask;
        uint16_t survivalMask;

        mutable std::unique_ptr<uint8_t[]> cells;
        mutable std::unique_ptr<uint8_t[]> next;

        static Rule clampRule(Rule rule);
        static uint16_t birthMaskFor(Rule rule);
        static uint16_t survivalMaskFor(Rule rule);
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_LIFEVARIANTPATTERN_H
