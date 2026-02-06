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

#ifndef POLAR_SHADER_SPECS_PIPELINESTEP_H
#define POLAR_SHADER_SPECS_PIPELINESTEP_H

#include <memory>
#include "transforms/base/Transforms.h"
#include "transforms/palette/PaletteTransform.h"

namespace PolarShader {
    enum class PipelineStepKind {
        Cartesian,
        Polar,
        UV,
        Palette,
        ToCartesian,
        ToPolar
    };

    // PolarPipeline.h (or a dedicated header if you prefer)
    struct PipelineStep {
        PipelineStepKind kind;
        std::unique_ptr<CartesianTransform> cartesianTransform;
        std::unique_ptr<PolarTransform> polarTransform;
        std::unique_ptr<UVTransform> uvTransform;
        std::unique_ptr<PaletteTransform> paletteTransform;

        // Factories for domain conversion steps (no transform pointers)
        static PipelineStep toPolar() {
            PipelineStep s;
            s.kind = PipelineStepKind::ToPolar;
            return s;
        }

        static PipelineStep toCartesian() {
            PipelineStep s;
            s.kind = PipelineStepKind::ToCartesian;
            return s;
        }

        // Factories for transform steps (exactly one pointer is set)
        static PipelineStep cartesian(std::unique_ptr<CartesianTransform> t) {
            PipelineStep s;
            s.kind = PipelineStepKind::Cartesian;
            s.cartesianTransform = std::move(t);
            return s;
        }

        static PipelineStep polar(std::unique_ptr<PolarTransform> t) {
            PipelineStep s;
            s.kind = PipelineStepKind::Polar;
            s.polarTransform = std::move(t);
            return s;
        }

        static PipelineStep uv(std::unique_ptr<UVTransform> t) {
            PipelineStep s;
            s.kind = PipelineStepKind::UV;
            s.uvTransform = std::move(t);
            return s;
        }

        static PipelineStep palette(std::unique_ptr<PaletteTransform> t) {
            PipelineStep s;
            s.kind = PipelineStepKind::Palette;
            s.paletteTransform = std::move(t);
            return s;
        }
    };
}
#endif //POLAR_SHADER_SPECS_PIPELINESTEP_H
