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

#include "renderer/pipeline/patterns/LifeVariantPattern.h"
#include <new>

namespace PolarShader {
    namespace {
        // Bit n set means "count n is in the set". Neighbour count is 0..8.
        constexpr uint16_t countBit(uint8_t n) { return static_cast<uint16_t>(1u << n); }
    }

    LifeVariantPattern::Rule LifeVariantPattern::clampRule(Rule rule) {
        switch (rule) {
            case Rule::HighLife:
            case Rule::DayAndNight:
            case Rule::Seeds:
                return rule;
            default:
                return Rule::HighLife;
        }
    }

    uint16_t LifeVariantPattern::birthMaskFor(Rule rule) {
        switch (rule) {
            case Rule::HighLife: return countBit(3) | countBit(6);
            case Rule::DayAndNight: return countBit(3) | countBit(6) | countBit(7) | countBit(8);
            case Rule::Seeds: return countBit(2);
            default: return countBit(3) | countBit(6);
        }
    }

    uint16_t LifeVariantPattern::survivalMaskFor(Rule rule) {
        switch (rule) {
            case Rule::HighLife: return countBit(2) | countBit(3);
            case Rule::DayAndNight: return countBit(3) | countBit(4) | countBit(6) | countBit(7) | countBit(8);
            case Rule::Seeds: return 0u;
            default: return countBit(2) | countBit(3);
        }
    }

    LifeVariantPattern::LifeVariantPattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint16_t densityPermille,
        Rule rule
    ) : RasterAutomaton(stepIntervalMs, seed),
        densityPermille(raster::clampPermille(densityPermille)),
        birthMask(birthMaskFor(clampRule(rule))),
        survivalMask(survivalMaskFor(clampRule(rule))) {
    }

    bool LifeVariantPattern::allocate(uint16_t, uint16_t, uint32_t cellCount) const {
        std::unique_ptr<uint8_t[]> newCells(new (std::nothrow) uint8_t[cellCount]);
        std::unique_ptr<uint8_t[]> newNext(new (std::nothrow) uint8_t[cellCount]);
        if (!newCells || !newNext) return false;

        cells = std::move(newCells);
        next = std::move(newNext);
        return true;
    }

    void LifeVariantPattern::release() const {
        cells.reset();
        next.reset();
    }

    void LifeVariantPattern::seed(uint32_t generationSeed) const {
        if (!cells || !next) return;

        uint32_t rng = generationSeed == 0 ? 0xA5A5A5A5u : generationSeed;
        const uint32_t count = cellCount();
        for (uint32_t i = 0; i < count; ++i) {
            const uint16_t roll = static_cast<uint16_t>((raster::lcgNext(rng) >> 16) % 1000u);
            cells[i] = roll < densityPermille ? 1u : 0u;
            next[i] = 0u;
        }
    }

    bool LifeVariantPattern::step() const {
        if (!cells || !next || width() == 0 || height() == 0) return false;

        bool changed = false;
        for (uint16_t y = 0; y < height(); ++y) {
            for (uint16_t x = 0; x < width(); ++x) {
                const uint32_t idx = static_cast<uint32_t>(y) * width() + x;
                const uint8_t neighbours =
                    raster::countMooreState(cells.get(), x, y, width(), height(), 1u);
                const bool alive = cells[idx] != 0;
                const uint16_t mask = alive ? survivalMask : birthMask;
                const bool nextAlive = (mask & countBit(neighbours)) != 0;
                next[idx] = nextAlive ? 1u : 0u;
                changed = changed || nextAlive != alive;
            }
        }
        cells.swap(next);
        return changed;
    }

    RasterMap LifeVariantPattern::rasterLayer(const std::shared_ptr<PipelineContext> &context) const {
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
