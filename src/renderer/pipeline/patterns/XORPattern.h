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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_XORPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_XORPATTERN_H

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include <memory>

namespace PolarShader {
    // "Munching squares": the (x XOR y) texture ramp, scrolled over time.
    class XORPattern : public UVPattern {
    public:
        explicit XORPattern(
            uint8_t gridSize = 16,
            uint16_t speed = 40
        );

        void advanceFrame(u0x16 progress, TimeMillis elapsedMs) override;
        UVMap layer(const std::shared_ptr<PipelineContext>& context) const override;

    private:
        struct State {
            uint8_t gridSize;
            uint16_t speed;
            uint16_t phase{0u};
        };

        std::shared_ptr<State> state;

        static uint8_t clampGridSize(uint8_t value);

        struct Functor;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_XORPATTERN_H
