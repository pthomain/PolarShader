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

#include "renderer/pipeline/patterns/LangtonAntPattern.h"
#include <new>

namespace PolarShader {
    namespace {
        constexpr uint16_t kMinAnts = 1u;
        constexpr uint16_t kMaxAnts = 8u;

        uint16_t clampAntCount(uint16_t value) {
            if (value < kMinAnts) return kMinAnts;
            if (value > kMaxAnts) return kMaxAnts;
            return value;
        }
    }

    LangtonAntPattern::LangtonAntPattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint16_t antCount
    ) : RasterAutomaton(stepIntervalMs, seed),
        antCount(clampAntCount(antCount)) {
    }

    bool LangtonAntPattern::allocate(uint16_t, uint16_t, uint32_t cellCount) const {
        std::unique_ptr<uint8_t[]> newCells(new (std::nothrow) uint8_t[cellCount]);
        std::unique_ptr<Ant[]> newAnts(new (std::nothrow) Ant[antCount]);
        if (!newCells || !newAnts) return false;

        cells = std::move(newCells);
        ants = std::move(newAnts);
        return true;
    }

    void LangtonAntPattern::release() const {
        cells.reset();
        ants.reset();
    }

    void LangtonAntPattern::seed(uint32_t generationSeed) const {
        if (!cells || !ants) return;

        rng = generationSeed == 0 ? 0xA5A5A5A5u : generationSeed;
        const uint32_t count = cellCount();
        for (uint32_t i = 0; i < count; ++i) {
            cells[i] = 0u;
        }

        for (uint16_t i = 0; i < antCount; ++i) {
            ants[i].x = static_cast<uint16_t>((raster::lcgNext(rng) >> 16) % width());
            ants[i].y = static_cast<uint16_t>((raster::lcgNext(rng) >> 16) % height());
            ants[i].dir = static_cast<uint8_t>((raster::lcgNext(rng) >> 16) & 0x3u);
        }
    }

    bool LangtonAntPattern::step() const {
        if (!cells || !ants || width() == 0 || height() == 0) return false;

        for (uint16_t i = 0; i < antCount; ++i) {
            Ant &ant = ants[i];
            const uint32_t idx = static_cast<uint32_t>(ant.y) * width() + ant.x;
            if (cells[idx]) {
                // White cell: turn right, then clear.
                ant.dir = static_cast<uint8_t>((ant.dir + 1u) & 0x3u);
                cells[idx] = 0u;
            } else {
                // Black cell: turn left, then paint.
                ant.dir = static_cast<uint8_t>((ant.dir + 3u) & 0x3u);
                cells[idx] = 1u;
            }

            switch (ant.dir) {
                case 0: // up
                    ant.y = ant.y == 0 ? static_cast<uint16_t>(height() - 1) : static_cast<uint16_t>(ant.y - 1);
                    break;
                case 1: // right
                    ant.x = static_cast<uint16_t>(ant.x + 1u >= width() ? 0u : ant.x + 1u);
                    break;
                case 2: // down
                    ant.y = static_cast<uint16_t>(ant.y + 1u >= height() ? 0u : ant.y + 1u);
                    break;
                default: // left
                    ant.x = ant.x == 0 ? static_cast<uint16_t>(width() - 1) : static_cast<uint16_t>(ant.x - 1);
                    break;
            }
        }

        // Ants always move, so the board always changes; never auto-reseed.
        return true;
    }

    RasterMap LangtonAntPattern::rasterLayer(const std::shared_ptr<PipelineContext> &context) const {
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
