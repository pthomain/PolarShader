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

#ifndef POLAR_SHADER_TRANSFORMS_BENDTRANSFORM_H
#define POLAR_SHADER_TRANSFORMS_BENDTRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "../modulators/signals/ScalarSignals.h"

namespace PolarShader {
    /**
     * Applies a simple bend/curve to Cartesian coordinates: x' = x + k * y^2 (and/or y' = y + k' * x^2).
     * Bend coefficients are bounded modulators mapped to an internal range. Inputs are clamped to int32;
     * wraps modulo 2^32.
     *
     * Parameters: kx, ky (BoundedScalarSignal, Q0.16).
     * Recommended order: after basic scale/shear, before polar conversion, to impose curvature on the domain.
     */
    class BendTransform : public CartesianTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        BendTransform(BoundedScalarSignal kx, BoundedScalarSignal ky);

        void advanceFrame(TimeMillis timeInMillis) override;

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //POLAR_SHADER_TRANSFORMS_BENDTRANSFORM_H
