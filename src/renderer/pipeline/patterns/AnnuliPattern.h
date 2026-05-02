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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_ANNULIPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_ANNULIPATTERN_H

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include <memory>

namespace PolarShader {
    /**
     * Concentric annuli sweep-fill pattern.
     *
     * A polar reinterpretation of CD77's box-fill effect: cells are partitioned by
     * (ringIndex, sliceIndex) over `ringCount` annuli x `slicesPerRing` angular slices.
     * A flat counter advances one cell per `stepIntervalMs`, fills that cell with
     * an angle-based intensity, and on completion holds for `holdMs` before resetting.
     *
     * `reverse=false` fills outermost ring first (outside -> in).
     * `reverse=true`  fills innermost ring first (inside  -> out).
     */
    class AnnuliPattern : public UVPattern {
    private:
        struct State;
        struct UVAnnuliFunctor;
        std::shared_ptr<State> state;

    public:
        AnnuliPattern(
            uint8_t ringCount = 8,
            uint8_t slicesPerRing = 32,
            bool reverse = false,
            uint16_t stepIntervalMs = 80,
            uint16_t holdMs = 800
        );

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;

        UVMap layer(const std::shared_ptr<PipelineContext> &context) const override;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_ANNULIPATTERN_H
