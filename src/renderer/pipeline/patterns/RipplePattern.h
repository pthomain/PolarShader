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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_RIPPLEPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_RIPPLEPATTERN_H

#include "renderer/pipeline/patterns/base/RasterAutomaton.h"
#include <memory>

namespace PolarShader {
    // Integer water-ripple simulation: a two-buffer discrete wave equation
    // over the grid. Droplets are injected periodically and their circular
    // waves interfere and decay according to the damping parameter.
    class RipplePattern : public RasterAutomaton {
    public:
        explicit RipplePattern(
            uint16_t stepIntervalMs = 40,
            uint16_t seed = 0,
            uint8_t damping = 6
        );

        RasterMap rasterLayer(const std::shared_ptr<PipelineContext>& context) const override;

    protected:
        bool allocate(uint16_t width, uint16_t height, uint32_t cellCount) const override;
        void release() const override;
        void seed(uint32_t generationSeed) const override;
        bool step() const override;

    private:
        uint8_t damping;

        mutable std::unique_ptr<int16_t[]> cur;
        mutable std::unique_ptr<int16_t[]> prev;
        mutable uint32_t waveRng{0};
        mutable uint32_t stepCounter{0};

        void injectDroplet() const;

        static uint8_t clampDamping(uint8_t value);
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_RIPPLEPATTERN_H
