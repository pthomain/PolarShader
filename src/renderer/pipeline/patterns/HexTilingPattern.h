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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_CARTESIAN_HEXTILINGPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_CARTESIAN_HEXTILINGPATTERN_H

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include "renderer/pipeline/signals/SignalTypes.h"

namespace PolarShader {
    /**
     * @brief Regular hexagon tiling with an N-coloring that guarantees no adjacent matches.
     *
     * The hex size is the radius from the center to any corner (pointy-top orientation).
     * Colors repeat using a (q - r) modulo scheme on axial coordinates, which ensures
     * all 6 neighbors are different when colorCount >= 3.
     */
    class HexTilingPattern : public UVPattern {
        // Stateless sampler used by the pipeline.
        struct UVHexTilingFunctor;

        // Fixed-point axial/cube rounding result for a single sample.
        struct HexAxial {
            int32_t q_raw;
            int32_t r_raw;
            int32_t s_raw;
            int32_t rx;
            int32_t ry;
            int32_t rz;
        };

        Sf16Signal hex_radius_signal;
        uint16_t hex_radius_u16;
        uint8_t color_count;
        uint16_t softness_u16;
        int32_t softness_raw;

    public:
        explicit HexTilingPattern(
            uint16_t hexRadius = 32,
            uint8_t colorCount = 3,
            uint16_t edgeSoftness = 0
        );

        explicit HexTilingPattern(
            Sf16Signal hexRadius,
            uint8_t colorCount = 3,
            uint16_t edgeSoftness = 0
        );

        UVMap layer(const std::shared_ptr<PipelineContext> &context) const override;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_CARTESIAN_HEXTILINGPATTERN_H
