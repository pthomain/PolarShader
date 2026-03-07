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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_REACTIONDIFFUSIONPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_REACTIONDIFFUSIONPATTERN_H

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include <memory>

namespace PolarShader {
    /**
     * @brief Gray-Scott reaction-diffusion simulation sampled as a UV pattern.
     *
     * Maintains a U/V chemical concentration grid updated each frame via
     * advanceFrame(). The compiled sampler keeps a shared reference to the
     * simulation state and bilinearly samples the current front V buffer.
     *
     * The renderer advances the simulation before sampling starts, so both
     * RP2040 cores only read the front buffer during render.
     */
    class ReactionDiffusionPattern : public UVPattern {
    public:
        // Well-known Gray-Scott parameter presets.
        enum class Preset : uint8_t {
            Spots,    // f=0.025, k=0.058 – isolated blobs
            Stripes,  // f=0.040, k=0.060 – band/stripe patterns
            Coral,    // f=0.055, k=0.062 – coral/sponge (default)
            Worms,    // f=0.018, k=0.055 – worm-like filaments
        };

    private:
        struct State {
            uint8_t width;
            uint8_t height;
            std::unique_ptr<uint16_t[]> u;
            std::unique_ptr<uint16_t[]> v;
            std::unique_ptr<uint16_t[]> u_next;
            std::unique_ptr<uint16_t[]> v_next;
            int32_t du;   // diffusion rate U, Q16 (fixed: 0.2)
            int32_t dv;   // diffusion rate V, Q16 (fixed: 0.1)
            uint32_t f;   // feed rate, Q16
            uint32_t k;   // kill rate, Q16
            uint32_t framesSinceSeed = 0;
        };

        std::shared_ptr<State> state;
        uint8_t stepsPerFrame;

            static void step(State& s);
            static void seed(State& s);
            struct RDFunctor;

    public:
        explicit ReactionDiffusionPattern(
            Preset preset = Preset::Coral,
            uint8_t width = 20,
            uint8_t height = 20,
            uint8_t stepsPerFrame = 4
        );

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;
        UVMap layer(const std::shared_ptr<PipelineContext>& context) const override;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_REACTIONDIFFUSIONPATTERN_H
