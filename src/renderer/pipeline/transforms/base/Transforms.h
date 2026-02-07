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

#ifndef POLAR_SHADER_TRANSFORMS_BASE_TRANSFORMS_H
#define POLAR_SHADER_TRANSFORMS_BASE_TRANSFORMS_H

#include <memory>
#include <utility>
#include "Layers.h"
#include "renderer/pipeline/PipelineContext.h"
#include "renderer/pipeline/maths/units/Units.h"

namespace PolarShader {
    class FrameTransform {
    public:
        virtual ~FrameTransform() = default;

        virtual void advanceFrame(TimeMillis timeInMillis) {
        };

        virtual void setContext(std::shared_ptr<PipelineContext> context) { this->context = std::move(context); }

    protected:
        std::shared_ptr<PipelineContext> context;
    };

    /**
     * @brief Standard interface for all spatial transformations in the unified UV pipeline.
     * 
     * UVTransform is the target architecture for all transforms.
     */
    class UVTransform : public virtual FrameTransform {
    public:
        UVTransform() = default;

        /** @brief Transforms one UV layer into another. */
        virtual UVLayer operator()(const UVLayer &layer) const = 0;
    };
}

#endif //POLAR_SHADER_TRANSFORMS_BASE_TRANSFORMS_H
