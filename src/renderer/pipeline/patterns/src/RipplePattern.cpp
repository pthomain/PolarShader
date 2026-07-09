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

#include "renderer/pipeline/patterns/RipplePattern.h"
#include <new>

namespace PolarShader {
    namespace {
        // Initial droplet height and how often (in steps) a new droplet lands.
        constexpr int16_t kDropletAmplitude = 4000;
        constexpr uint32_t kDropletIntervalSteps = 24u;
    }

    uint8_t RipplePattern::clampDamping(uint8_t value) {
        if (value < 1) return 1;
        if (value > 8) return 8;
        return value;
    }

    RipplePattern::RipplePattern(
        uint16_t stepIntervalMs,
        uint16_t seed,
        uint8_t damping
    ) : RasterAutomaton(stepIntervalMs, seed),
        damping(clampDamping(damping)) {
    }

    bool RipplePattern::allocate(uint16_t, uint16_t, uint32_t cellCount) const {
        std::unique_ptr<int16_t[]> newCur(new (std::nothrow) int16_t[cellCount]);
        std::unique_ptr<int16_t[]> newPrev(new (std::nothrow) int16_t[cellCount]);
        if (!newCur || !newPrev) return false;

        cur = std::move(newCur);
        prev = std::move(newPrev);
        return true;
    }

    void RipplePattern::release() const {
        cur.reset();
        prev.reset();
    }

    void RipplePattern::injectDroplet() const {
        const uint16_t x = static_cast<uint16_t>((raster::lcgNext(waveRng) >> 16) % width());
        const uint16_t y = static_cast<uint16_t>((raster::lcgNext(waveRng) >> 16) % height());
        cur[static_cast<uint32_t>(y) * width() + x] = kDropletAmplitude;
    }

    void RipplePattern::seed(uint32_t generationSeed) const {
        if (!cur || !prev) return;

        const uint32_t count = cellCount();
        for (uint32_t i = 0; i < count; ++i) {
            cur[i] = 0;
            prev[i] = 0;
        }
        waveRng = generationSeed == 0 ? 0xA5A5A5A5u : generationSeed;
        stepCounter = 0;
        injectDroplet();
    }

    bool RipplePattern::step() const {
        if (!cur || !prev || width() == 0 || height() == 0) return false;

        const uint16_t w = width();
        const uint16_t h = height();

        bool anyEnergy = false;
        for (uint16_t y = 0; y < h; ++y) {
            const uint16_t yUp = y == 0 ? static_cast<uint16_t>(h - 1) : static_cast<uint16_t>(y - 1);
            const uint16_t yDown = y == h - 1 ? 0 : static_cast<uint16_t>(y + 1);
            for (uint16_t x = 0; x < w; ++x) {
                const uint16_t xLeft = x == 0 ? static_cast<uint16_t>(w - 1) : static_cast<uint16_t>(x - 1);
                const uint16_t xRight = x == w - 1 ? 0 : static_cast<uint16_t>(x + 1);
                const uint32_t idx = static_cast<uint32_t>(y) * w + x;

                const int32_t sum =
                    static_cast<int32_t>(cur[static_cast<uint32_t>(yUp) * w + x]) +
                    cur[static_cast<uint32_t>(yDown) * w + x] +
                    cur[static_cast<uint32_t>(y) * w + xLeft] +
                    cur[static_cast<uint32_t>(y) * w + xRight];

                int32_t value = (sum >> 1) - prev[idx];
                value -= value >> damping;
                if (value > 32767) value = 32767;
                if (value < -32768) value = -32768;
                prev[idx] = static_cast<int16_t>(value);
                if (value != 0) anyEnergy = true;
            }
        }

        cur.swap(prev);

        ++stepCounter;
        if (stepCounter % kDropletIntervalSteps == 0) {
            injectDroplet();
            anyEnergy = true;
        }
        return anyEnergy;
    }

    RasterMap RipplePattern::rasterLayer(const std::shared_ptr<PipelineContext> &context) const {
        configure(context);
        return [this](const RasterPoint &point) {
            if (!ready() || !point.valid || !cur) {
                return PatternNormU16(0);
            }
            if (point.width != width() || point.height != height() ||
                point.x >= width() || point.y >= height()) {
                return PatternNormU16(0);
            }

            const uint32_t idx = static_cast<uint32_t>(point.y) * width() + point.x;
            int32_t mag = cur[idx];
            if (mag < 0) mag = -mag;
            // Amplitude range 0..4095 maps to the full 0..F16_MAX brightness.
            const uint16_t value = mag > 4095 ? static_cast<uint16_t>(F16_MAX)
                                              : static_cast<uint16_t>(mag << 4);
            return PatternNormU16(value);
        };
    }
}
