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

namespace PolarShader {
    struct PolarGradient::PolarGradientFunctor {
        const Axis axisValue;

        explicit PolarGradientFunctor(Axis axis) : axisValue(axis) {
        }

        PatternNormU16 operator()(FracQ0_16 angle, FracQ0_16 radius) const {
            return PatternNormU16(axisValue == Axis::Radius ? raw(radius) : raw(angle));
        }
    };

    PolarGradient::PolarGradient(Axis axis)
        : axis(axis) {
    }

    PolarLayer PolarGradient::layer(const std::shared_ptr<PipelineContext> &context) const {
        return PolarGradientFunctor(axis);
    }
}
