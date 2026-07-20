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

#include "renderer/pipeline/patterns/WireWorldPattern.h"
#include <new>

namespace PolarShader {
    namespace {
        // Cell states.
        constexpr uint8_t kWwEmpty = 0u;
        constexpr uint8_t kWwHead = 1u;
        constexpr uint8_t kWwTail = 2u;
        constexpr uint8_t kWwConductor = 3u;

        // Fraction of conductor cells seeded as electron heads.
        constexpr uint16_t kWwHeadSeedPermille = 40u;

        // Hue (0..255) each live state renders as before the palette tint.
        constexpr uint8_t kWwConductorHue = 32u; // amber
        constexpr uint8_t kWwTailHue = 0u;        // red
        constexpr uint8_t kWwHeadHue = 40u;       // yellow
    }

    WireWorldPattern::WireWorldPattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint16_t densityPermille
    ) : RasterAutomaton(stepIntervalMs, seed),
        densityPermille(raster::clampPermille(densityPermille)) {
    }

    bool WireWorldPattern::allocate(uint16_t, uint16_t, uint32_t cellCount) const {
        std::unique_ptr<uint8_t[]> newCells(new (std::nothrow) uint8_t[cellCount]);
        std::unique_ptr<uint8_t[]> newNext(new (std::nothrow) uint8_t[cellCount]);
        if (!newCells || !newNext) return false;

        cells = std::move(newCells);
        next = std::move(newNext);
        return true;
    }

    void WireWorldPattern::release() const {
        cells.reset();
        next.reset();
    }

    void WireWorldPattern::seed(uint32_t generationSeed) const {
        if (!cells || !next) return;

        rng = generationSeed == 0 ? 0xA5A5A5A5u : generationSeed;
        const uint32_t count = cellCount();
        for (uint32_t i = 0; i < count; ++i) {
            const uint16_t conductorRoll =
                static_cast<uint16_t>((raster::lcgNext(rng) >> 16) % 1000u);
            if (conductorRoll < densityPermille) {
                const uint16_t headRoll =
                    static_cast<uint16_t>((raster::lcgNext(rng) >> 16) % 1000u);
                cells[i] = headRoll < kWwHeadSeedPermille ? kWwHead : kWwConductor;
            } else {
                cells[i] = kWwEmpty;
            }
            next[i] = kWwEmpty;
        }
    }

    bool WireWorldPattern::step() const {
        if (!cells || !next || width() == 0 || height() == 0) return false;

        bool liveElectrons = false;
        for (uint16_t y = 0; y < height(); ++y) {
            for (uint16_t x = 0; x < width(); ++x) {
                const uint32_t idx = static_cast<uint32_t>(y) * width() + x;
                const uint8_t current = cells[idx];
                uint8_t result;
                if (current == kWwHead) {
                    result = kWwTail;
                } else if (current == kWwTail) {
                    result = kWwConductor;
                } else if (current == kWwConductor) {
                    const uint8_t headNeighbours =
                        raster::countMooreState(cells.get(), x, y, width(), height(), kWwHead);
                    result = (headNeighbours == 1u || headNeighbours == 2u) ? kWwHead : kWwConductor;
                } else {
                    result = kWwEmpty;
                }
                if (result == kWwHead || result == kWwTail) liveElectrons = true;
                next[idx] = result;
            }
        }
        cells.swap(next);
        // No electrons left means the board is frozen: reseed for a fresh mesh.
        return liveElectrons;
    }

    RasterMap WireWorldPattern::rasterLayer(const std::shared_ptr<PipelineContext> &context) const {
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
            switch (cells[idx]) {
                case kWwHead: return PatternNormU16(U0X16_MAX);
                case kWwTail: return PatternNormU16(U0X16_MAX * 3u / 4u);
                case kWwConductor: return PatternNormU16(U0X16_MAX / 4u);
                default: return PatternNormU16(0);
            }
        };
    }

    RasterColourMap WireWorldPattern::rasterColourLayer(const std::shared_ptr<PipelineContext> &context) const {
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
            switch (cells[idx]) {
                case kWwHead:
                    return PaletteSample{
                        PatternNormU16(raster::hue8ToPatternRaw(kWwHeadHue)),
                        PatternNormU16(U0X16_MAX)
                    };
                case kWwTail:
                    return PaletteSample{
                        PatternNormU16(raster::hue8ToPatternRaw(kWwTailHue)),
                        PatternNormU16(U0X16_MAX * 3u / 4u)
                    };
                case kWwConductor:
                    return PaletteSample{
                        PatternNormU16(raster::hue8ToPatternRaw(kWwConductorHue)),
                        PatternNormU16(U0X16_MAX / 4u)
                    };
                default:
                    return PaletteSample{};
            }
        };
    }
}
