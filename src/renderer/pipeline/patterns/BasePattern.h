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
#include <utility>

namespace PolarShader {

    enum class PatternDomain {
        Cartesian,
        Polar
    };

    enum class PatternKind {
        Noise,
        Gradient,
        SpaceFillingCurve,
        Fractal,
        PeriodicTiling,
        DistanceField,
        Custom
    };

    /**
     * @brief Describes the nature of a pattern's output value.
     * This contract helps determine if and how normalization should be applied.
     */
    enum class OutputSemantic {
        /// @brief A continuous value field, like noise. Should be normalized to full range.
        Field,
        /// @brief A binary or soft-edged mask. Should be 0 or 65535, with optional smooth transitions.
        Mask,
        /// @brief A categorical ID, like for Voronoi cells. Should not be normalized.
        Id,
        /// @brief A signed distance field. Centered at 0, with distance stored in the value.
        SignedField
    };

    class PatternBase {
    public:
        virtual ~PatternBase() = default;

        PatternDomain domain() const { return domainValue; }
        PatternKind kind() const { return kindValue; }
        virtual OutputSemantic semantic() const { return semanticValue; }

    protected:
        PatternBase(PatternDomain domain, PatternKind kind, OutputSemantic semantic)
            : domainValue(domain),
              kindValue(kind),
              semanticValue(semantic) {
        }

    private:
        PatternDomain domainValue;
        PatternKind kindValue;
        OutputSemantic semanticValue;
    };

    class CartesianPattern : public PatternBase {
    public:
        explicit CartesianPattern(PatternKind kind, OutputSemantic semantic = OutputSemantic::Field)
            : PatternBase(PatternDomain::Cartesian, kind, semantic) {
        }

        virtual CartesianLayer layer() const = 0;
    };

    class PolarPattern : public PatternBase {
    public:
        explicit PolarPattern(PatternKind kind, OutputSemantic semantic = OutputSemantic::Field)
            : PatternBase(PatternDomain::Polar, kind, semantic) {
        }

        virtual PolarLayer layer() const = 0;
    };

    class SimpleCartesianPattern : public CartesianPattern {
        CartesianLayer layerValue;

    public:
        SimpleCartesianPattern(
            CartesianLayer layer,
            PatternKind kind = PatternKind::Custom,
            OutputSemantic semantic = OutputSemantic::Field
        ) : CartesianPattern(kind, semantic),
            layerValue(std::move(layer)) {
        }

        CartesianLayer layer() const override {
            return layerValue;
        }
    };

    class SimplePolarPattern : public PolarPattern {
        PolarLayer layerValue;

    public:
        SimplePolarPattern(
            PolarLayer layer,
            PatternKind kind = PatternKind::Custom,
            OutputSemantic semantic = OutputSemantic::Field
        ) : PolarPattern(kind, semantic),
            layerValue(std::move(layer)) {
        }

        PolarLayer layer() const override {
            return layerValue;
        }
    };
}

#endif // POLAR_SHADER_PIPELINE_BASEPATTERN_H
