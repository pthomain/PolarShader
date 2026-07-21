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

#include "renderer/pipeline/patterns/base/RasterAutomaton.h"

namespace PolarShader {
    namespace raster {
        uint32_t lcgNext(uint32_t &state) {
            state = (state * 1664525u) + 1013904223u;
            return state;
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

        uint16_t clampPermille(uint16_t value) {
            return value > 1000u ? 1000u : value;
        }

        uint16_t hue8ToPatternRaw(uint8_t hue) {
            return (static_cast<uint16_t>(hue) << 8) | hue;
        }

        uint8_t countMooreState(
            const uint8_t *cells,
            uint16_t x,
            uint16_t y,
            uint16_t width,
            uint16_t height,
            uint8_t targetState
        ) {
            if (!cells || width == 0 || height == 0) return 0;

            const uint16_t xLeft = x == 0 ? static_cast<uint16_t>(width - 1) : static_cast<uint16_t>(x - 1);
            const uint16_t xRight = x == width - 1 ? 0 : static_cast<uint16_t>(x + 1);
            const uint16_t yUp = y == 0 ? static_cast<uint16_t>(height - 1) : static_cast<uint16_t>(y - 1);
            const uint16_t yDown = y == height - 1 ? 0 : static_cast<uint16_t>(y + 1);

            const uint32_t rowUp = static_cast<uint32_t>(yUp) * width;
            const uint32_t rowMid = static_cast<uint32_t>(y) * width;
            const uint32_t rowDown = static_cast<uint32_t>(yDown) * width;

            uint8_t count = 0;
            count += cells[rowUp + xLeft] == targetState;
            count += cells[rowUp + x] == targetState;
            count += cells[rowUp + xRight] == targetState;
            count += cells[rowMid + xLeft] == targetState;
            count += cells[rowMid + xRight] == targetState;
            count += cells[rowDown + xLeft] == targetState;
            count += cells[rowDown + x] == targetState;
            count += cells[rowDown + xRight] == targetState;
            return count;
        }
    }

    namespace {
        constexpr uint8_t kMaxCatchUpStepsPerFrame = 4;
    }

    RasterAutomaton::RasterAutomaton(uint16_t stepIntervalMs, uint16_t seed)
        : stepIntervalMs(stepIntervalMs), seedParam(seed) {
    }

    void RasterAutomaton::seedInitial() const {
        baseSeed = seedParam == 0 ? raster::randomSeed32() : seedParam;
        generation = 0;
        hasLastElapsed = false;
        lastElapsedMs = 0;
        accumulatedMs = 0;
        seed(raster::seedForGeneration(baseSeed, generation));
    }

    void RasterAutomaton::reseed() const {
        if (seedParam == 0) {
            baseSeed = raster::randomSeed32();
            generation = 0;
        } else {
            ++generation;
        }
        seed(raster::seedForGeneration(baseSeed, generation));
        accumulatedMs = 0;
    }

    void RasterAutomaton::configure(const std::shared_ptr<PipelineContext> &context) const {
        const RasterDisplayInfo display = context ? context->rasterDisplay : RasterDisplayInfo{};

        if (!display.valid || display.width == 0 || display.height == 0 || display.cellCount == 0) {
            if (!grid.warnedNoRaster) {
                Serial.println("RasterAutomaton requires a raster display; rendering black.");
                grid.warnedNoRaster = true;
            }
            grid.ready = false;
            release();
            grid.width = 0;
            grid.height = 0;
            grid.cellCount = 0;
            return;
        }

        if (display.cellCount > POLAR_SHADER_MAX_RASTER_CELLS) {
            if (!grid.warnedCapacity) {
                Serial.println("RasterAutomaton raster display exceeds POLAR_SHADER_MAX_RASTER_CELLS; rendering black.");
                grid.warnedCapacity = true;
            }
            grid.ready = false;
            release();
            grid.width = display.width;
            grid.height = display.height;
            grid.cellCount = display.cellCount;
            return;
        }

        if (grid.ready && grid.width == display.width && grid.height == display.height &&
            grid.cellCount == display.cellCount) {
            return;
        }

        if (!allocate(display.width, display.height, display.cellCount)) {
            if (!grid.warnedAllocation) {
                Serial.println("RasterAutomaton failed to allocate raster buffers; rendering black.");
                grid.warnedAllocation = true;
            }
            grid.ready = false;
            release();
            grid.width = display.width;
            grid.height = display.height;
            grid.cellCount = display.cellCount;
            return;
        }

        grid.width = display.width;
        grid.height = display.height;
        grid.cellCount = display.cellCount;
        grid.ready = true;
        grid.warnedNoRaster = false;
        grid.warnedCapacity = false;
        grid.warnedAllocation = false;
        seedInitial();
    }

    void RasterAutomaton::advanceFrame(u0x16 progress, TimeMillis elapsedMs) {
        (void) progress;
        if (!grid.ready) return;

        if (!hasLastElapsed || elapsedMs < lastElapsedMs) {
            // Elapsed time can wrap/reset when a scene duration loops; keep the
            // current board instead of reseeding, so loops do not visibly pop.
            hasLastElapsed = true;
            lastElapsedMs = elapsedMs;
            accumulatedMs = 0;
            return;
        }

        const TimeMillis deltaMs = elapsedMs - lastElapsedMs;
        lastElapsedMs = elapsedMs;

        if (stepIntervalMs == 0) {
            if (!step()) reseed();
            return;
        }

        accumulatedMs += deltaMs;
        const TimeMillis maxAccumulatedMs =
            static_cast<TimeMillis>(stepIntervalMs) * kMaxCatchUpStepsPerFrame;
        if (accumulatedMs > maxAccumulatedMs) {
            accumulatedMs = maxAccumulatedMs;
        }

        while (accumulatedMs >= stepIntervalMs) {
            accumulatedMs -= stepIntervalMs;
            if (!step()) reseed();
        }
    }
}
