//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2023 Pierre Thomain

/*
 * This file is part of LED Segments.
 *
 * LED Segments is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LED Segments is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LED Segments. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LED_SEGMENTS_SPECS_PIPELINESTEP_H
#define LED_SEGMENTS_SPECS_PIPELINESTEP_H
#include <memory>

#include "transforms/base/Transforms.h"

namespace LEDSegments {
    enum class PipelineStepKind {
        Cartesian,
        Polar,
        ToCartesian,
        ToPolar
    };

    // PolarPipeline.h (or a dedicated header if you prefer)
    struct PipelineStep {
        PipelineStepKind kind;
        std::unique_ptr<CartesianTransform> cartesianTransform;
        std::unique_ptr<PolarTransform> polarTransform;

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
    };
}
#endif //LED_SEGMENTS_SPECS_PIPELINESTEP_H
