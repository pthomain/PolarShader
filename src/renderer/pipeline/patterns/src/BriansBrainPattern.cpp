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

#include "renderer/pipeline/patterns/BriansBrainPattern.h"
#include <new>

namespace PolarShader {
    namespace {
        // Cell states.
        constexpr uint8_t kOff = 0u;
        constexpr uint8_t kFiring = 1u;
        constexpr uint8_t kDying = 2u;
    }

    BriansBrainPattern::BriansBrainPattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint16_t densityPermille
    ) : RasterAutomaton(stepIntervalMs, seed),
        densityPermille(raster::clampPermille(densityPermille)) {
    }

    bool BriansBrainPattern::allocate(uint16_t, uint16_t, uint32_t cellCount) const {
        std::unique_ptr<uint8_t[]> newCells(new (std::nothrow) uint8_t[cellCount]);
        std::unique_ptr<uint8_t[]> newNext(new (std::nothrow) uint8_t[cellCount]);
        if (!newCells || !newNext) return false;

        cells = std::move(newCells);
        next = std::move(newNext);
        return true;
    }

    void BriansBrainPattern::release() const {
        cells.reset();
        next.reset();
    }

    void BriansBrainPattern::seed(uint32_t generationSeed) const {
        if (!cells || !next) return;

        uint32_t rng = generationSeed == 0 ? 0xA5A5A5A5u : generationSeed;
        const uint32_t count = cellCount();
        for (uint32_t i = 0; i < count; ++i) {
            const uint16_t roll = static_cast<uint16_t>((raster::lcgNext(rng) >> 16) % 1000u);
            cells[i] = roll < densityPermille ? kFiring : kOff;
            next[i] = kOff;
        }
    }

    bool BriansBrainPattern::step() const {
        if (!cells || !next || width() == 0 || height() == 0) return false;

        bool changed = false;
        for (uint16_t y = 0; y < height(); ++y) {
            for (uint16_t x = 0; x < width(); ++x) {
                const uint32_t idx = static_cast<uint32_t>(y) * width() + x;
                const uint8_t current = cells[idx];
                uint8_t result;
                if (current == kFiring) {
                    result = kDying;
                } else if (current == kDying) {
                    result = kOff;
                } else {
                    const uint8_t firing =
                        raster::countMooreState(cells.get(), x, y, width(), height(), kFiring);
                    result = firing == 2u ? kFiring : kOff;
                }
                next[idx] = result;
                changed = changed || result != current;
            }
        }
        cells.swap(next);
        return changed;
    }

    RasterMap BriansBrainPattern::rasterLayer(const std::shared_ptr<PipelineContext> &context) const {
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
            const uint8_t current = cells[idx];
            if (current == kFiring) return PatternNormU0x16(U0X16_MAX);
            if (current == kDying) return PatternNormU0x16(U0X16_MAX / 3u);
            return PatternNormU0x16(0);
        };
    }
}
