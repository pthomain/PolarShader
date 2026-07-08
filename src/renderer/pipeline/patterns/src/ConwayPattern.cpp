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
#include <algorithm>
#include <new>

namespace PolarShader {
    namespace {
        constexpr uint8_t kMaxCatchUpStepsPerFrame = 4;

        uint32_t lcgNext(uint32_t &state) {
            state = (state * 1664525u) + 1013904223u;
            return state;
        }

        uint16_t clampDensity(uint16_t densityPermille) {
            return densityPermille > 1000u ? 1000u : densityPermille;
        }

        uint32_t randomSeed32() {
            uint32_t value = (static_cast<uint32_t>(random16()) << 16) | random16();
            return value == 0 ? 0xA5A5A5A5u : value;
        }

        uint32_t seedForGeneration(uint32_t baseSeed, uint32_t generation) {
            if (generation == 0) return baseSeed == 0 ? 0xA5A5A5A5u : baseSeed;

            uint32_t mixed = baseSeed ^ (generation * 0x9E3779B9u);
            mixed ^= mixed >> 16;
            mixed *= 0x7FEB352Du;
            mixed ^= mixed >> 15;
            mixed *= 0x846CA68Bu;
            mixed ^= mixed >> 16;
            return mixed == 0 ? 0x6D2B79F5u : mixed;
        }

        uint16_t hue8ToPatternRaw(uint8_t hue) {
            return (static_cast<uint16_t>(hue) << 8) | hue;
        }

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
    ) : state(std::make_shared<State>()),
        stepIntervalMs(stepIntervalMs),
        seed(seed),
        densityPermille(clampDensity(densityPermille)) {
    }

    void ConwayPattern::seedState(State &s, uint32_t generationSeed, bool resetTiming) const {
        uint32_t cellRng = generationSeed == 0 ? 0xA5A5A5A5u : generationSeed;
        uint32_t hueRng = cellRng ^ 0x9E3779B9u;
        if (hueRng == 0) hueRng = 0x6D2B79F5u;
        const uint16_t density = clampDensity(densityPermille);
        for (uint32_t i = 0; i < s.cellCount; ++i) {
            const uint16_t roll = static_cast<uint16_t>((lcgNext(cellRng) >> 16) % 1000u);
            const uint8_t hue = static_cast<uint8_t>(lcgNext(hueRng) >> 24);
            s.cells[i] = roll < density ? 1u : 0u;
            s.next[i] = 0u;
            s.hues[i] = s.cells[i] ? hue : 0u;
            s.nextHues[i] = 0u;
        }
        if (resetTiming) {
            s.hasLastElapsed = false;
            s.lastElapsedMs = 0;
            s.accumulatedMs = 0;
        }
    }

    void ConwayPattern::seedInitialState(State &s) const {
        s.baseSeed = seed == 0 ? randomSeed32() : seed;
        s.generation = 0;
        seedState(s, seedForGeneration(s.baseSeed, s.generation), true);
    }

    void ConwayPattern::reseedState(State &s) const {
        if (seed == 0) {
            s.baseSeed = randomSeed32();
            s.generation = 0;
        } else {
            ++s.generation;
        }
        seedState(s, seedForGeneration(s.baseSeed, s.generation), false);
        s.accumulatedMs = 0;
    }

    void ConwayPattern::configureState(const RasterDisplayInfo &display) const {
        State &s = *state;

        if (!display.valid || display.width == 0 || display.height == 0 || display.cellCount == 0) {
            if (!s.warnedNoRaster) {
                Serial.println("ConwayPattern requires a raster display; rendering black.");
                s.warnedNoRaster = true;
            }
            s.ready = false;
            s.cells.reset();
            s.next.reset();
            s.hues.reset();
            s.nextHues.reset();
            s.width = 0;
            s.height = 0;
            s.cellCount = 0;
            return;
        }

        if (display.cellCount > POLAR_SHADER_MAX_RASTER_CELLS) {
            if (!s.warnedCapacity) {
                Serial.println("ConwayPattern raster display exceeds POLAR_SHADER_MAX_RASTER_CELLS; rendering black.");
                s.warnedCapacity = true;
            }
            s.ready = false;
            s.cells.reset();
            s.next.reset();
            s.hues.reset();
            s.nextHues.reset();
            s.width = display.width;
            s.height = display.height;
            s.cellCount = display.cellCount;
            return;
        }

        if (s.ready && s.width == display.width && s.height == display.height && s.cellCount == display.cellCount) {
            return;
        }

        std::unique_ptr<uint8_t[]> cells(new (std::nothrow) uint8_t[display.cellCount]);
        std::unique_ptr<uint8_t[]> next(new (std::nothrow) uint8_t[display.cellCount]);
        std::unique_ptr<uint8_t[]> hues(new (std::nothrow) uint8_t[display.cellCount]);
        std::unique_ptr<uint8_t[]> nextHues(new (std::nothrow) uint8_t[display.cellCount]);
        if (!cells || !next || !hues || !nextHues) {
            if (!s.warnedAllocation) {
                Serial.println("ConwayPattern failed to allocate raster buffers; rendering black.");
                s.warnedAllocation = true;
            }
            s.ready = false;
            s.cells.reset();
            s.next.reset();
            s.hues.reset();
            s.nextHues.reset();
            s.width = display.width;
            s.height = display.height;
            s.cellCount = display.cellCount;
            return;
        }

        s.width = display.width;
        s.height = display.height;
        s.cellCount = display.cellCount;
        s.cells = std::move(cells);
        s.next = std::move(next);
        s.hues = std::move(hues);
        s.nextHues = std::move(nextHues);
        s.ready = true;
        s.warnedNoRaster = false;
        s.warnedCapacity = false;
        s.warnedAllocation = false;
        seedInitialState(s);
    }

    RasterMap ConwayPattern::rasterLayer(const std::shared_ptr<PipelineContext> &context) const {
        configureState(context ? context->rasterDisplay : RasterDisplayInfo{});
        return [s = state.get()](const RasterPoint &point) {
            if (!s || !s->ready || !point.valid || !s->cells) {
                return PatternNormU16(0);
            }
            if (point.width != s->width || point.height != s->height ||
                point.x >= s->width || point.y >= s->height) {
                return PatternNormU16(0);
            }

            const uint32_t idx = static_cast<uint32_t>(point.y) * s->width + point.x;
            return s->cells[idx] ? PatternNormU16(F16_MAX) : PatternNormU16(0);
        };
    }

    RasterColourMap ConwayPattern::rasterColourLayer(const std::shared_ptr<PipelineContext> &context) const {
        configureState(context ? context->rasterDisplay : RasterDisplayInfo{});
        return [s = state.get()](const RasterPoint &point) {
            if (!s || !s->ready || !point.valid || !s->cells || !s->hues) {
                return PaletteSample{};
            }
            if (point.width != s->width || point.height != s->height ||
                point.x >= s->width || point.y >= s->height) {
                return PaletteSample{};
            }

            const uint32_t idx = static_cast<uint32_t>(point.y) * s->width + point.x;
            if (!s->cells[idx]) {
                return PaletteSample{};
            }

            return PaletteSample{
                PatternNormU16(hue8ToPatternRaw(s->hues[idx])),
                PatternNormU16(F16_MAX)
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

    void ConwayPattern::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void)progress;
        State &s = *state;
        if (!s.ready || !s.cells || !s.next || !s.hues || !s.nextHues) return;

        if (!s.hasLastElapsed || elapsedMs < s.lastElapsedMs) {
            // Elapsed time can wrap/reset when a scene duration loops; keep the
            // current board instead of reseeding, so loops do not visibly pop.
            s.hasLastElapsed = true;
            s.lastElapsedMs = elapsedMs;
            s.accumulatedMs = 0;
            return;
        }

        const TimeMillis deltaMs = elapsedMs - s.lastElapsedMs;
        s.lastElapsedMs = elapsedMs;

        if (stepIntervalMs == 0) {
            if (stepCellsAndHues(s.cells.get(), s.hues.get(), s.next.get(), s.nextHues.get(), s.width, s.height)) {
                s.cells.swap(s.next);
                s.hues.swap(s.nextHues);
            } else {
                reseedState(s);
            }
            return;
        }

        s.accumulatedMs += deltaMs;
        const TimeMillis maxAccumulatedMs =
            static_cast<TimeMillis>(stepIntervalMs) * kMaxCatchUpStepsPerFrame;
        if (s.accumulatedMs > maxAccumulatedMs) {
            s.accumulatedMs = maxAccumulatedMs;
        }

        while (s.accumulatedMs >= stepIntervalMs) {
            s.accumulatedMs -= stepIntervalMs;
            if (stepCellsAndHues(s.cells.get(), s.hues.get(), s.next.get(), s.nextHues.get(), s.width, s.height)) {
                s.cells.swap(s.next);
                s.hues.swap(s.nextHues);
            } else {
                reseedState(s);
            }
        }
    }
}
