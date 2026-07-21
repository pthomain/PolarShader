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

#include "renderer/pipeline/patterns/ConwayPattern.h"
#include <new>

namespace PolarShader {
    namespace {
        int16_t shortestHueDelta(uint8_t base, uint8_t hue) {
            int16_t delta = static_cast<int16_t>(hue) - static_cast<int16_t>(base);
            if (delta > 127) delta -= 256;
            if (delta < -128) delta += 256;
            return delta;
        }

        void accumulateLiveNeighbour(
            const uint8_t *current,
            const uint8_t *hues,
            uint32_t idx,
            uint8_t &neighbours,
            uint8_t &hueCount,
            uint8_t &baseHue,
            int16_t &deltaSum
        ) {
            if (current[idx] == 0) return;

            ++neighbours;
            const uint8_t hue = hues[idx];
            if (hueCount == 0) {
                baseHue = hue;
            } else {
                deltaSum += shortestHueDelta(baseHue, hue);
            }
            ++hueCount;
        }

        uint8_t averagedHue(uint8_t baseHue, int16_t deltaSum, uint8_t hueCount) {
            if (hueCount == 0) return baseHue;
            return static_cast<uint8_t>(static_cast<int16_t>(baseHue) + (deltaSum / hueCount));
        }

        bool stepCellsAndHues(
            const uint8_t *current,
            const uint8_t *hues,
            uint8_t *next,
            uint8_t *nextHues,
            uint16_t width,
            uint16_t height
        ) {
            if (!current || !hues || !next || !nextHues || width == 0 || height == 0) return false;

            bool changed = false;

            for (uint16_t y = 0; y < height; ++y) {
                const uint16_t yUp = y == 0 ? static_cast<uint16_t>(height - 1) : static_cast<uint16_t>(y - 1);
                const uint16_t yDown = y == height - 1 ? 0 : static_cast<uint16_t>(y + 1);

                for (uint16_t x = 0; x < width; ++x) {
                    const uint16_t xLeft = x == 0 ? static_cast<uint16_t>(width - 1) : static_cast<uint16_t>(x - 1);
                    const uint16_t xRight = x == width - 1 ? 0 : static_cast<uint16_t>(x + 1);
                    const uint32_t idx = static_cast<uint32_t>(y) * width + x;

                    uint8_t neighbours = 0;
                    uint8_t hueCount = 0;
                    uint8_t baseHue = hues[idx];
                    int16_t deltaSum = 0;

                    accumulateLiveNeighbour(current, hues, static_cast<uint32_t>(yUp) * width + xLeft,
                                             neighbours, hueCount, baseHue, deltaSum);
                    accumulateLiveNeighbour(current, hues, static_cast<uint32_t>(yUp) * width + x,
                                             neighbours, hueCount, baseHue, deltaSum);
                    accumulateLiveNeighbour(current, hues, static_cast<uint32_t>(yUp) * width + xRight,
                                             neighbours, hueCount, baseHue, deltaSum);
                    accumulateLiveNeighbour(current, hues, static_cast<uint32_t>(y) * width + xLeft,
                                             neighbours, hueCount, baseHue, deltaSum);
                    accumulateLiveNeighbour(current, hues, static_cast<uint32_t>(y) * width + xRight,
                                             neighbours, hueCount, baseHue, deltaSum);
                    accumulateLiveNeighbour(current, hues, static_cast<uint32_t>(yDown) * width + xLeft,
                                             neighbours, hueCount, baseHue, deltaSum);
                    accumulateLiveNeighbour(current, hues, static_cast<uint32_t>(yDown) * width + x,
                                             neighbours, hueCount, baseHue, deltaSum);
                    accumulateLiveNeighbour(current, hues, static_cast<uint32_t>(yDown) * width + xRight,
                                             neighbours, hueCount, baseHue, deltaSum);

                    const bool alive = current[idx] != 0;
                    const bool nextAlive = neighbours == 3 || (alive && neighbours == 2);
                    next[idx] = nextAlive ? 1u : 0u;
                    changed = changed || nextAlive != alive;

                    if (!nextAlive) {
                        nextHues[idx] = 0;
                    } else if (alive) {
                        nextHues[idx] = hues[idx];
                    } else {
                        nextHues[idx] = averagedHue(baseHue, deltaSum, hueCount);
                    }
                }
            }
            return changed;
        }
    }

    ConwayPattern::ConwayPattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint16_t densityPermille
    ) : RasterAutomaton(stepIntervalMs, seed),
        densityPermille(raster::clampPermille(densityPermille)) {
    }

    bool ConwayPattern::allocate(uint16_t, uint16_t, uint32_t cellCount) const {
        std::unique_ptr<uint8_t[]> newCells(new (std::nothrow) uint8_t[cellCount]);
        std::unique_ptr<uint8_t[]> newNext(new (std::nothrow) uint8_t[cellCount]);
        std::unique_ptr<uint8_t[]> newHues(new (std::nothrow) uint8_t[cellCount]);
        std::unique_ptr<uint8_t[]> newNextHues(new (std::nothrow) uint8_t[cellCount]);
        if (!newCells || !newNext || !newHues || !newNextHues) return false;

        cells = std::move(newCells);
        next = std::move(newNext);
        hues = std::move(newHues);
        nextHues = std::move(newNextHues);
        return true;
    }

    void ConwayPattern::release() const {
        cells.reset();
        next.reset();
        hues.reset();
        nextHues.reset();
    }

    void ConwayPattern::seed(uint32_t generationSeed) const {
        if (!cells || !next || !hues || !nextHues) return;

        uint32_t cellRng = generationSeed == 0 ? 0xA5A5A5A5u : generationSeed;
        uint32_t hueRng = cellRng ^ 0x9E3779B9u;
        if (hueRng == 0) hueRng = 0x6D2B79F5u;
        const uint32_t count = cellCount();
        for (uint32_t i = 0; i < count; ++i) {
            const uint16_t roll = static_cast<uint16_t>((raster::lcgNext(cellRng) >> 16) % 1000u);
            const uint8_t hue = static_cast<uint8_t>(raster::lcgNext(hueRng) >> 24);
            cells[i] = roll < densityPermille ? 1u : 0u;
            next[i] = 0u;
            hues[i] = cells[i] ? hue : 0u;
            nextHues[i] = 0u;
        }
    }

    bool ConwayPattern::step() const {
        if (!stepCellsAndHues(cells.get(), hues.get(), next.get(), nextHues.get(), width(), height())) {
            return false;
        }
        cells.swap(next);
        hues.swap(nextHues);
        return true;
    }

    RasterMap ConwayPattern::rasterLayer(const std::shared_ptr<PipelineContext> &context) const {
        configure(context);
        return [this](const RasterPoint &point) {
            if (!ready() || !point.valid || !cells) {
                return PatternNormU0x16(0);
            }
            if (point.width != width() || point.height != height() ||
                point.x >= width() || point.y >= height()) {
                return PatternNormU0x16(0);
            }

            const uint32_t idx = static_cast<uint32_t>(point.y) * width() + point.x;
            return cells[idx] ? PatternNormU0x16(U0X16_MAX) : PatternNormU0x16(0);
        };
    }

    RasterColourMap ConwayPattern::rasterColourLayer(const std::shared_ptr<PipelineContext> &context) const {
        configure(context);
        return [this](const RasterPoint &point) {
            if (!ready() || !point.valid || !cells || !hues) {
                return PaletteSample{};
            }
            if (point.width != width() || point.height != height() ||
                point.x >= width() || point.y >= height()) {
                return PaletteSample{};
            }

            const uint32_t idx = static_cast<uint32_t>(point.y) * width() + point.x;
            if (!cells[idx]) {
                return PaletteSample{};
            }

            return PaletteSample{
                PatternNormU0x16(raster::hue8ToPatternRaw(hues[idx])),
                PatternNormU0x16(U0X16_MAX)
            };
        };
    }

    void ConwayPattern::stepCells(
        const uint8_t *current,
        uint8_t *next,
        uint16_t width,
        uint16_t height
    ) {
        if (!current || !next || width == 0 || height == 0) return;

        for (uint16_t y = 0; y < height; ++y) {
            const uint16_t yUp = y == 0 ? static_cast<uint16_t>(height - 1) : static_cast<uint16_t>(y - 1);
            const uint16_t yDown = y == height - 1 ? 0 : static_cast<uint16_t>(y + 1);

            for (uint16_t x = 0; x < width; ++x) {
                const uint16_t xLeft = x == 0 ? static_cast<uint16_t>(width - 1) : static_cast<uint16_t>(x - 1);
                const uint16_t xRight = x == width - 1 ? 0 : static_cast<uint16_t>(x + 1);

                const uint32_t idx = static_cast<uint32_t>(y) * width + x;
                const uint8_t neighbours =
                    current[static_cast<uint32_t>(yUp) * width + xLeft] +
                    current[static_cast<uint32_t>(yUp) * width + x] +
                    current[static_cast<uint32_t>(yUp) * width + xRight] +
                    current[static_cast<uint32_t>(y) * width + xLeft] +
                    current[static_cast<uint32_t>(y) * width + xRight] +
                    current[static_cast<uint32_t>(yDown) * width + xLeft] +
                    current[static_cast<uint32_t>(yDown) * width + x] +
                    current[static_cast<uint32_t>(yDown) * width + xRight];

                const bool alive = current[idx] != 0;
                next[idx] = (neighbours == 3 || (alive && neighbours == 2)) ? 1u : 0u;
            }
        }
    }
}
