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

#include "BasePattern.h"

namespace PolarShader {
    PatternDomain BasePattern::domain() const {
        return domainValue;
    }

    void BasePattern::setContext(std::shared_ptr<PipelineContext> context) {
        this->context = std::move(context);
    }

    BasePattern::BasePattern(PatternDomain domain)
        : domainValue(domain) {
    }

    CartesianLayer BasePattern::defaultCartesianLayer() {
        return [](CartQ24_8, CartQ24_8) { return PatternNormU16(0); };
    }

    PolarLayer BasePattern::defaultPolarLayer() {
        return [](FracQ0_16, FracQ0_16) { return PatternNormU16(0); };
    }

    CartesianPattern::CartesianPattern()
        : BasePattern(PatternDomain::Cartesian),
          layerValue(defaultCartesianLayer()) {
    }

    CartesianPattern::CartesianPattern(CartesianLayer layer)
        : BasePattern(PatternDomain::Cartesian),
          layerValue(std::move(layer)) {
        if (!layerValue) {
            layerValue = defaultCartesianLayer();
        }
    }

    CartesianLayer CartesianPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        return layerValue;
    }

    PolarPattern::PolarPattern()
        : BasePattern(PatternDomain::Polar),
          layerValue(defaultPolarLayer()) {
    }

    PolarPattern::PolarPattern(PolarLayer layer)
        : BasePattern(PatternDomain::Polar),
          layerValue(std::move(layer)) {
        if (!layerValue) {
            layerValue = defaultPolarLayer();
        }
    }

    PolarLayer PolarPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        return layerValue;
    }
}
