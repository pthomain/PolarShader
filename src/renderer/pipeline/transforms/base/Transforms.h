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

#include "Layers.h"
#include <renderer/pipeline/units/Range.h>
#include <utility>

namespace PolarShader {
    class FrameTransform {
    public:
        virtual ~FrameTransform() = default;

        virtual void advanceFrame(TimeMillis timeInMillis) {};
    };

    class CartesianTransform : public FrameTransform {
    public:
        explicit CartesianTransform(Range range = Range::scalarRange(0, 0)) : range(range) {}

        virtual CartesianLayer operator()(const CartesianLayer &layer) const = 0;

    protected:
        int32_t mapScalar(SFracQ0_16 t) const { return range.mapScalar(t); }
        SPoint32 mapCartesian(SFracQ0_16 direction, SFracQ0_16 velocity) const {
            return range.mapCartesian(direction, velocity);
        }

        Range range;
    };

    class PolarTransform : public FrameTransform {
    public:
        explicit PolarTransform(Range range = Range::polarRange(FracQ0_16(0), FracQ0_16(FRACT_Q0_16_MAX)))
            : range(range) {}

        virtual PolarLayer operator()(const PolarLayer &layer) const = 0;

    protected:
        FracQ0_16 mapPolar(SFracQ0_16 t) const { return range.map(t); }

        Range range;
    };
}

#endif //POLAR_SHADER_TRANSFORMS_BASE_TRANSFORMS_H
