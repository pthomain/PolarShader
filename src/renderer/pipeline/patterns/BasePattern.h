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

    class BasePattern {
    public:
        virtual ~BasePattern() = default;

        PatternDomain domain() const { return domainValue; }

    protected:
        explicit BasePattern(PatternDomain domain)
            : domainValue(domain) {
        }

        static CartesianLayer defaultCartesianLayer() {
            return [](CartQ24_8, CartQ24_8, uint32_t) { return PatternNormU16(0); };
        }

        static PolarLayer defaultPolarLayer() {
            return [](FracQ0_16, FracQ0_16, uint32_t) { return PatternNormU16(0); };
        }

    private:
        PatternDomain domainValue;
    };

    class CartesianPattern : public BasePattern {
        CartesianLayer layerValue;

    public:
        CartesianPattern() : BasePattern(PatternDomain::Cartesian),
                             layerValue(defaultCartesianLayer()) {
        }

        explicit CartesianPattern(CartesianLayer layer) : BasePattern(PatternDomain::Cartesian),
                                                          layerValue(std::move(layer)) {
            if (!layerValue) {
                layerValue = defaultCartesianLayer();
            }
        }

        virtual CartesianLayer layer() const {
            return layerValue;
        }
    };

    class PolarPattern : public BasePattern {
        PolarLayer layerValue;

    public:
        PolarPattern() : BasePattern(PatternDomain::Polar),
                         layerValue(defaultPolarLayer()) {
        }

        explicit PolarPattern(PolarLayer layer) : BasePattern(PatternDomain::Polar),
                                                  layerValue(std::move(layer)) {
            if (!layerValue) {
                layerValue = defaultPolarLayer();
            }
        }

        virtual PolarLayer layer() const {
            return layerValue;
        }
    };
}

#endif // POLAR_SHADER_PIPELINE_BASEPATTERN_H
