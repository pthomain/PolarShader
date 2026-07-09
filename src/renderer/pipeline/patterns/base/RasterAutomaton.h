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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_RASTERAUTOMATON_H
#define POLAR_SHADER_PIPELINE_PATTERNS_RASTERAUTOMATON_H

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include <memory>

namespace PolarShader {
#ifndef POLAR_SHADER_MAX_RASTER_CELLS
#define POLAR_SHADER_MAX_RASTER_CELLS 4096u
#endif

    namespace raster {
        // Small deterministic LCG shared by every raster automaton so that a
        // given (seed, generation) always produces the same board.
        uint32_t lcgNext(uint32_t &state);

        // Non-zero 32-bit runtime seed pulled from FastLED's PRNG.
        uint32_t randomSeed32();

        // Derive a stable, well-mixed per-generation seed from a base seed.
        uint32_t seedForGeneration(uint32_t baseSeed, uint32_t generation);

        // Clamp a per-mille probability to [0, 1000].
        uint16_t clampPermille(uint16_t value);

        // Expand an 8-bit hue to the pattern's 16-bit normalized range.
        uint16_t hue8ToPatternRaw(uint8_t hue);

        // Count how many of a cell's 8 Moore neighbours (toroidal wrap) hold
        // exactly targetState. Shared by every cellular-automaton subclass.
        uint8_t countMooreState(
            const uint8_t *cells,
            uint16_t x,
            uint16_t y,
            uint16_t width,
            uint16_t height,
            uint8_t targetState
        );
    }

    /**
     * @brief Shared lifecycle for stateful pixel-grid ("raster") patterns.
     *
     * Owns the display-driven allocation lifecycle, the frame-timing /
     * catch-up driver, and the deterministic reseed machinery. Subclasses own
     * their own cell buffers (of whatever element type they need) and supply
     * four hooks: allocate(), release(), seed() and step().
     */
    class RasterAutomaton : public UVPattern {
    public:
        PatternDomain domain() const override { return PatternDomain::RasterGrid; }
        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;

    protected:
        RasterAutomaton(uint16_t stepIntervalMs, uint16_t seed);

        // Reconcile the pattern with the current display. Call at the top of
        // rasterLayer()/rasterColourLayer() before sampling.
        void configure(const std::shared_ptr<PipelineContext> &context) const;

        bool ready() const { return grid.ready; }
        uint16_t width() const { return grid.width; }
        uint16_t height() const { return grid.height; }
        uint32_t cellCount() const { return grid.cellCount; }
        uint16_t seedValue() const { return seedParam; }

        // Allocate (or resize) cell buffers for a width*height grid. Returns
        // false on allocation failure. Called only when dimensions change.
        virtual bool allocate(uint16_t width, uint16_t height, uint32_t cellCount) const = 0;

        // Release all cell buffers (invalid or over-capacity display).
        virtual void release() const = 0;

        // Populate the buffers for a fresh generation from generationSeed.
        virtual void seed(uint32_t generationSeed) const = 0;

        // Advance one generation. Return true if the board changed; returning
        // false triggers a deterministic reseed so the pattern never stalls.
        virtual bool step() const = 0;

    private:
        struct Grid {
            bool ready{false};
            uint16_t width{0};
            uint16_t height{0};
            uint32_t cellCount{0};
            bool warnedNoRaster{false};
            bool warnedCapacity{false};
            bool warnedAllocation{false};
        };

        mutable Grid grid;
        mutable bool hasLastElapsed{false};
        mutable TimeMillis lastElapsedMs{0};
        mutable TimeMillis accumulatedMs{0};
        mutable uint32_t baseSeed{0};
        mutable uint32_t generation{0};
        uint16_t stepIntervalMs;
        uint16_t seedParam;

        void seedInitial() const;
        void reseed() const;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_RASTERAUTOMATON_H
