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

#include "renderer/pipeline/patterns/ElementaryCAPattern.h"
#include <cstring>
#include <new>

namespace PolarShader {
    ElementaryCAPattern::ElementaryCAPattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint8_t rule
    ) : RasterAutomaton(stepIntervalMs, seed),
        rule(rule) {
    }

    bool ElementaryCAPattern::allocate(uint16_t width, uint16_t, uint32_t cellCount) const {
        std::unique_ptr<uint8_t[]> newCells(new (std::nothrow) uint8_t[cellCount]);
        std::unique_ptr<uint8_t[]> newRow(new (std::nothrow) uint8_t[width]);
        if (!newCells || !newRow) return false;

        cells = std::move(newCells);
        rowBuf = std::move(newRow);
        return true;
    }

    void ElementaryCAPattern::release() const {
        cells.reset();
        rowBuf.reset();
    }

    void ElementaryCAPattern::seed(uint32_t generationSeed) const {
        if (!cells) return;

        const uint32_t count = cellCount();
        for (uint32_t i = 0; i < count; ++i) cells[i] = 0u;

        // Seed only the bottom row so history can grow upward from it.
        uint32_t rng = generationSeed == 0 ? 0xA5A5A5A5u : generationSeed;
        const uint32_t bottom = static_cast<uint32_t>(height() - 1) * width();
        for (uint16_t x = 0; x < width(); ++x) {
            cells[bottom + x] = static_cast<uint8_t>((raster::lcgNext(rng) >> 16) & 1u);
        }
    }

    bool ElementaryCAPattern::step() const {
        if (!cells || !rowBuf || width() == 0 || height() == 0) return false;

        const uint16_t w = width();
        const uint16_t h = height();
        const uint32_t bottom = static_cast<uint32_t>(h - 1) * w;

        bool changed = false;
        for (uint16_t x = 0; x < w; ++x) {
            const uint16_t xLeft = x == 0 ? static_cast<uint16_t>(w - 1) : static_cast<uint16_t>(x - 1);
            const uint16_t xRight = x == w - 1 ? 0 : static_cast<uint16_t>(x + 1);
            const uint8_t triple = static_cast<uint8_t>(
                (cells[bottom + xLeft] << 2) |
                (cells[bottom + x] << 1) |
                cells[bottom + xRight]);
            const uint8_t newCell = static_cast<uint8_t>((rule >> triple) & 1u);
            rowBuf[x] = newCell;
            changed = changed || newCell != cells[bottom + x];
        }

        // The bottom row can reach a fixed point (e.g. all-zero) while the
        // accumulated history is still scrolling up the display. Only report
        // "no change" — which triggers a reseed that erases the grid — once the
        // grid is genuinely static: every row uniform and the bottom row stable.
        if (!changed) {
            for (uint16_t y = 0; y + 1 < h && !changed; ++y) {
                const uint32_t rowOff = static_cast<uint32_t>(y) * w;
                for (uint16_t x = 0; x < w; ++x) {
                    if (cells[rowOff + x] != cells[bottom + x]) { changed = true; break; }
                }
            }
        }

        // Scroll every row up by one, then drop the freshly computed row in.
        if (h > 1) {
            std::memmove(cells.get(), cells.get() + w, static_cast<size_t>(bottom));
        }
        std::memcpy(cells.get() + bottom, rowBuf.get(), w);
        return changed;
    }

    RasterMap ElementaryCAPattern::rasterLayer(const std::shared_ptr<PipelineContext> &context) const {
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
            return cells[idx] ? PatternNormU16(F16_MAX) : PatternNormU16(0);
        };
    }
}
