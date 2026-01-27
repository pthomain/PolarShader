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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_CARTESIAN_CARTESIANPERIODICTILINGPATTERN_H
#define POLAR_SHADER_PIPELINE_PATTERNS_CARTESIAN_CARTESIANPERIODICTILINGPATTERN_H

#include "renderer/pipeline/patterns/BasePattern.h"

namespace PolarShader {
    class CartesianPeriodicTilingPattern : public CartesianPattern {
        struct PeriodicTilingStub {
            PatternNormU16 operator()(CartQ24_8, CartQ24_8) const {
                return PatternNormU16(0);
            }
        };

    public:
        CartesianPeriodicTilingPattern()
            : CartesianPattern(PatternKind::PeriodicTiling, OutputSemantic::Mask) {
        }

        CartesianLayer layer() const override {
            return PeriodicTilingStub{};
        }
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_CARTESIAN_CARTESIANPERIODICTILINGPATTERN_H
