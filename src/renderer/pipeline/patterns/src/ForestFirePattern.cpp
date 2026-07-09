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

#if defined(ARDUINO) || defined(__EMSCRIPTEN__)
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif

#include "renderer/pipeline/patterns/ForestFirePattern.h"
#include <new>

namespace PolarShader {
    namespace {
        // Cell states.
        constexpr uint8_t kEmpty = 0u;
        constexpr uint8_t kTree = 1u;
        constexpr uint8_t kBurning = 2u;

        // Initial tree coverage when a generation is seeded.
        constexpr uint16_t kInitialTreePermille = 500u;

        // Hue (0..255) each live state renders as before the palette tint.
        constexpr uint8_t kTreeHue = 96u;    // green
        constexpr uint8_t kBurningHue = 16u; // red-orange
    }

    ForestFirePattern::ForestFirePattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint16_t growthPermille,
        uint16_t lightningPermille
    ) : RasterAutomaton(stepIntervalMs, seed),
        growthPermille(raster::clampPermille(growthPermille)),
        lightningPermille(raster::clampPermille(lightningPermille)) {
    }

    bool ForestFirePattern::allocate(uint16_t, uint16_t, uint32_t cellCount) const {
        std::unique_ptr<uint8_t[]> newCells(new (std::nothrow) uint8_t[cellCount]);
        std::unique_ptr<uint8_t[]> newNext(new (std::nothrow) uint8_t[cellCount]);
        if (!newCells || !newNext) return false;

        cells = std::move(newCells);
        next = std::move(newNext);
        return true;
    }

    void ForestFirePattern::release() const {
        cells.reset();
        next.reset();
    }

    void ForestFirePattern::seed(uint32_t generationSeed) const {
        if (!cells || !next) return;

        rng = generationSeed == 0 ? 0xA5A5A5A5u : generationSeed;
        const uint32_t count = cellCount();
        for (uint32_t i = 0; i < count; ++i) {
            const uint16_t roll = static_cast<uint16_t>((raster::lcgNext(rng) >> 16) % 1000u);
            cells[i] = roll < kInitialTreePermille ? kTree : kEmpty;
            next[i] = kEmpty;
        }
    }

    bool ForestFirePattern::step() const {
        if (!cells || !next || width() == 0 || height() == 0) return false;

        bool changed = false;
        for (uint16_t y = 0; y < height(); ++y) {
            for (uint16_t x = 0; x < width(); ++x) {
                const uint32_t idx = static_cast<uint32_t>(y) * width() + x;
                const uint8_t current = cells[idx];
                uint8_t result;
                if (current == kBurning) {
                    result = kEmpty;
                } else if (current == kTree) {
                    const uint8_t burningNeighbours =
                        raster::countMooreState(cells.get(), x, y, width(), height(), kBurning);
                    if (burningNeighbours > 0) {
                        result = kBurning;
                    } else {
                        const uint16_t roll =
                            static_cast<uint16_t>((raster::lcgNext(rng) >> 16) % 1000u);
                        result = roll < lightningPermille ? kBurning : kTree;
                    }
                } else {
                    const uint16_t roll =
                        static_cast<uint16_t>((raster::lcgNext(rng) >> 16) % 1000u);
                    result = roll < growthPermille ? kTree : kEmpty;
                }
                next[idx] = result;
                changed = changed || result != current;
            }
        }
        cells.swap(next);
        return changed;
    }

    RasterMap ForestFirePattern::rasterLayer(const std::shared_ptr<PipelineContext> &context) const {
        configure(context);
        return [this](const RasterPoint &point) {
            if (!ready() || !point.valid || !cells) {
                return PatternNormU16(0);
            }
            if (point.width != width() || point.height != height() ||
                point.x >= width() || point.y >= height()) {
                return PatternNormU16(0);
            }

            const uint32_t idx = static_cast<uint32_t>(point.y) * width() + point.x;
            const uint8_t current = cells[idx];
            if (current == kBurning) return PatternNormU16(F16_MAX);
            if (current == kTree) return PatternNormU16(F16_MAX / 2u);
            return PatternNormU16(0);
        };
    }

    RasterColourMap ForestFirePattern::rasterColourLayer(const std::shared_ptr<PipelineContext> &context) const {
        configure(context);
        return [this](const RasterPoint &point) {
            if (!ready() || !point.valid || !cells) {
                return PaletteSample{};
            }
            if (point.width != width() || point.height != height() ||
                point.x >= width() || point.y >= height()) {
                return PaletteSample{};
            }

            const uint32_t idx = static_cast<uint32_t>(point.y) * width() + point.x;
            const uint8_t current = cells[idx];
            if (current == kBurning) {
                return PaletteSample{
                    PatternNormU16(raster::hue8ToPatternRaw(kBurningHue)),
                    PatternNormU16(F16_MAX)
                };
            }
            if (current == kTree) {
                return PaletteSample{
                    PatternNormU16(raster::hue8ToPatternRaw(kTreeHue)),
                    PatternNormU16(F16_MAX / 2u)
                };
            }
            return PaletteSample{};
        };
    }
}
