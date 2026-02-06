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

#include "PolarGradient.h"
#include "renderer/pipeline/maths/PolarMaths.h"

namespace PolarShader {
    struct PolarGradient::UVPolarGradientFunctor {
        const Axis axisValue;

        explicit UVPolarGradientFunctor(Axis axis) : axisValue(axis) {
        }

        PatternNormU16 operator()(UV uv) const {
            UV polar_uv = cartesianToPolarUV(uv);
            // U=Angle, V=Radius
            return PatternNormU16(axisValue == Axis::Radius ? static_cast<uint16_t>(raw(polar_uv.v)) : static_cast<uint16_t>(raw(polar_uv.u)));
        }
    };

    PolarGradient::PolarGradient(Axis axis)
        : axis(axis) {
    }

    UVLayer PolarGradient::layer(const std::shared_ptr<PipelineContext> &context) const {
        return UVPolarGradientFunctor(axis);
    }
}
