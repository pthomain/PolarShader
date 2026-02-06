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

#include "UVPattern.h"

namespace PolarShader {
    void UVPattern::setContext(std::shared_ptr<PipelineContext> context) {
        this->context = std::move(context);
    }

    UVPattern::UVPattern()
        : layerValue([](UV) { return PatternNormU16(0); }) {
    }

    UVPattern::UVPattern(UVLayer layer)
        : layerValue(std::move(layer)) {
        if (!layerValue) {
            layerValue = [](UV) { return PatternNormU16(0); };
        }
    }

    UVLayer UVPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        return layerValue;
    }
}