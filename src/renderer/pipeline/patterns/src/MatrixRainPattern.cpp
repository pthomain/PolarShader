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

#include "renderer/pipeline/patterns/MatrixRainPattern.h"
#include <new>

namespace PolarShader {
    uint8_t MatrixRainPattern::clampFade(uint8_t value) {
        return value < 1 ? 1 : value;
    }

    MatrixRainPattern::MatrixRainPattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint8_t fadeAmount
    ) : RasterAutomaton(stepIntervalMs, seed),
        fadeAmount(clampFade(fadeAmount)) {
    }

    bool MatrixRainPattern::allocate(uint16_t width, uint16_t, uint32_t cellCount) const {
        std::unique_ptr<uint8_t[]> newBrightness(new (std::nothrow) uint8_t[cellCount]);
        std::unique_ptr<uint16_t[]> newHeads(new (std::nothrow) uint16_t[width]);
        if (!newBrightness || !newHeads) return false;

        brightness = std::move(newBrightness);
        heads = std::move(newHeads);
        return true;
    }

    void MatrixRainPattern::release() const {
        brightness.reset();
        heads.reset();
    }

    void MatrixRainPattern::seed(uint32_t generationSeed) const {
        if (!brightness || !heads) return;

        const uint32_t count = cellCount();
        for (uint32_t i = 0; i < count; ++i) brightness[i] = 0u;

        uint32_t rng = generationSeed == 0 ? 0xA5A5A5A5u : generationSeed;
        for (uint16_t x = 0; x < width(); ++x) {
            heads[x] = static_cast<uint16_t>((raster::lcgNext(rng) >> 16) % height());
        }
    }

    bool MatrixRainPattern::step() const {
        if (!brightness || !heads || width() == 0 || height() == 0) return false;

        const uint32_t count = cellCount();
        for (uint32_t i = 0; i < count; ++i) {
            const uint8_t b = brightness[i];
            brightness[i] = b > fadeAmount ? static_cast<uint8_t>(b - fadeAmount) : 0u;
        }

        for (uint16_t x = 0; x < width(); ++x) {
            const uint16_t head = static_cast<uint16_t>((heads[x] + 1u) % height());
            heads[x] = head;
            brightness[static_cast<uint32_t>(head) * width() + x] = 255u;
        }
        return true;
    }

    RasterMap MatrixRainPattern::rasterLayer(const std::shared_ptr<PipelineContext> &context) const {
        configure(context);
        return [this](const RasterPoint &point) {
            if (!ready() || !point.valid || !brightness) {
                return PatternNormU0x16(0);
            }
            if (point.width != width() || point.height != height() ||
                point.x >= width() || point.y >= height()) {
                return PatternNormU0x16(0);
            }

            const uint32_t idx = static_cast<uint32_t>(point.y) * width() + point.x;
            return PatternNormU0x16(raster::hue8ToPatternRaw(brightness[idx]));
        };
    }
}
