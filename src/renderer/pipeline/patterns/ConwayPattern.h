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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_CONWAYPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_CONWAYPATTERN_H

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include <memory>

namespace PolarShader {
#ifndef POLAR_SHADER_MAX_RASTER_CELLS
#define POLAR_SHADER_MAX_RASTER_CELLS 4096u
#endif

    class ConwayPattern : public UVPattern {
    public:
        explicit ConwayPattern(
            uint16_t stepIntervalMs = 250,
            uint16_t seed = 0,
            uint16_t densityPermille = 350
        );

        PatternDomain domain() const override { return PatternDomain::RasterGrid; }
        bool emitsColour() const override { return true; }
        RasterMap rasterLayer(const std::shared_ptr<PipelineContext>& context) const override;
        RasterColourMap rasterColourLayer(const std::shared_ptr<PipelineContext>& context) const override;
        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;

        static void stepCells(
            const uint8_t *current,
            uint8_t *next,
            uint16_t width,
            uint16_t height
        );

    private:
        struct State {
            uint16_t width{0};
            uint16_t height{0};
            uint32_t cellCount{0};
            std::unique_ptr<uint8_t[]> cells;
            std::unique_ptr<uint8_t[]> next;
            std::unique_ptr<uint8_t[]> hues;
            std::unique_ptr<uint8_t[]> nextHues;
            bool ready{false};
            bool warnedNoRaster{false};
            bool warnedCapacity{false};
            bool warnedAllocation{false};
            bool hasLastElapsed{false};
            TimeMillis lastElapsedMs{0};
            TimeMillis accumulatedMs{0};
            uint32_t baseSeed{0};
            uint32_t generation{0};
        };

        mutable std::shared_ptr<State> state;
        uint16_t stepIntervalMs;
        uint16_t seed;
        uint16_t densityPermille;

        void configureState(const RasterDisplayInfo &display) const;
        void seedInitialState(State &s) const;
        void reseedState(State &s) const;
        void seedState(State &s, uint32_t generationSeed, bool resetTiming) const;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_CONWAYPATTERN_H
