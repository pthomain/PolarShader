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

#include "renderer/pipeline/patterns/CyclicCAPattern.h"
#include <new>

namespace PolarShader {
    uint8_t CyclicCAPattern::clampNumStates(uint8_t value) {
        if (value < 3) return 3;
        if (value > 16) return 16;
        return value;
    }

    uint8_t CyclicCAPattern::clampThreshold(uint8_t value) {
        if (value < 1) return 1;
        if (value > 8) return 8;
        return value;
    }

    CyclicCAPattern::CyclicCAPattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint8_t numStates,
        uint8_t threshold
    ) : RasterAutomaton(stepIntervalMs, seed),
        numStates(clampNumStates(numStates)),
        threshold(clampThreshold(threshold)) {
    }

    bool CyclicCAPattern::allocate(uint16_t, uint16_t, uint32_t cellCount) const {
        std::unique_ptr<uint8_t[]> newCells(new (std::nothrow) uint8_t[cellCount]);
        std::unique_ptr<uint8_t[]> newNext(new (std::nothrow) uint8_t[cellCount]);
        if (!newCells || !newNext) return false;

        cells = std::move(newCells);
        next = std::move(newNext);
        return true;
    }

    void CyclicCAPattern::release() const {
        cells.reset();
        next.reset();
    }

    void CyclicCAPattern::seed(uint32_t generationSeed) const {
        if (!cells || !next) return;

        uint32_t rng = generationSeed == 0 ? 0xA5A5A5A5u : generationSeed;
        const uint32_t count = cellCount();
        for (uint32_t i = 0; i < count; ++i) {
            cells[i] = static_cast<uint8_t>((raster::lcgNext(rng) >> 24) % numStates);
            next[i] = 0u;
        }
    }

    bool CyclicCAPattern::step() const {
        if (!cells || !next || width() == 0 || height() == 0) return false;

        bool changed = false;
        for (uint16_t y = 0; y < height(); ++y) {
            for (uint16_t x = 0; x < width(); ++x) {
                const uint32_t idx = static_cast<uint32_t>(y) * width() + x;
                const uint8_t current = cells[idx];
                const uint8_t successor = static_cast<uint8_t>((current + 1u) % numStates);
                const uint8_t match =
                    raster::countMooreState(cells.get(), x, y, width(), height(), successor);
                if (match >= threshold) {
                    next[idx] = successor;
                    changed = true;
                } else {
                    next[idx] = current;
                }
            }
        }
        cells.swap(next);
        return changed;
    }

    RasterMap CyclicCAPattern::rasterLayer(const std::shared_ptr<PipelineContext> &context) const {
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
            const uint16_t value =
                static_cast<uint16_t>((static_cast<uint32_t>(cells[idx]) * U0X16_MAX) / (numStates - 1u));
            return PatternNormU0x16(value);
        };
    }

    RasterColourMap CyclicCAPattern::rasterColourLayer(const std::shared_ptr<PipelineContext> &context) const {
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
            const uint8_t hue8 =
                static_cast<uint8_t>((static_cast<uint16_t>(cells[idx]) * 256u) / numStates);
            return PaletteSample{
                PatternNormU0x16(raster::hue8ToPatternRaw(hue8)),
                PatternNormU0x16(U0X16_MAX)
            };
        };
    }
}
