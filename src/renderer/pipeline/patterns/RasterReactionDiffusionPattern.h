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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_RASTERREACTIONDIFFUSIONPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_RASTERREACTIONDIFFUSIONPATTERN_H

#include "renderer/pipeline/patterns/base/RasterAutomaton.h"
#include <memory>

namespace PolarShader {
    // Gray-Scott reaction-diffusion on the pixel grid. Two chemical fields U
    // and V diffuse and react (U + 2V -> 3V) with feed/kill terms; the V field
    // drives the display intensity. Different feed/kill presets settle into
    // spots, stripes, coral or worm textures. Deterministic per (seed,
    // generation): reseeded automatically when the field saturates.
    class RasterReactionDiffusionPattern : public RasterAutomaton {
    public:
        enum class Preset : uint8_t {
            Spots = 0,
            Stripes = 1,
            Coral = 2,
            Worms = 3,
        };

        explicit RasterReactionDiffusionPattern(
            uint8_t preset = 0,
            uint16_t stepIntervalMs = 33,
            uint16_t seed = 0,
            uint16_t iterationsPerStep = 4
        );

        RasterMap rasterLayer(const std::shared_ptr<PipelineContext>& context) const override;

    protected:
        bool allocate(uint16_t width, uint16_t height, uint32_t cellCount) const override;
        void release() const override;
        void seed(uint32_t generationSeed) const override;
        bool step() const override;

    private:
        void euler() const;

        uint16_t f;
        uint16_t k;
        uint16_t iterationsPerStep;

        mutable std::unique_ptr<uint16_t[]> u;
        mutable std::unique_ptr<uint16_t[]> v;
        mutable std::unique_ptr<uint16_t[]> uNext;
        mutable std::unique_ptr<uint16_t[]> vNext;
        mutable uint32_t rng{0};
        mutable uint8_t saturatedStreak{0};
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_RASTERREACTIONDIFFUSIONPATTERN_H
