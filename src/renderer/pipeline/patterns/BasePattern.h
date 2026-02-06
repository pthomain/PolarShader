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

#ifndef POLAR_SHADER_PIPELINE_BASEPATTERN_H
#define POLAR_SHADER_PIPELINE_BASEPATTERN_H

#include "renderer/pipeline/transforms/base/Layers.h"
#include "renderer/pipeline/PipelineContext.h"
#include <memory>
#include <utility>

namespace PolarShader {
    enum class PatternDomain {
        UV
    };

    class BasePattern {
    public:
        virtual ~BasePattern() = default;

        PatternDomain domain() const;

        virtual void setContext(std::shared_ptr<PipelineContext> context);

    protected:
        explicit BasePattern(PatternDomain domain);

    private:
        std::shared_ptr<PipelineContext> context;
        PatternDomain domainValue;
    };

    /**
     * @brief Standard interface for all spatial patterns in the unified UV pipeline.
     */
    class UVPattern : public BasePattern {
        UVLayer layerValue;

    public:
        UVPattern();

        explicit UVPattern(UVLayer layer);

        virtual UVLayer layer(const std::shared_ptr<PipelineContext> &context) const;
    };
}

#endif // POLAR_SHADER_PIPELINE_BASEPATTERN_H
