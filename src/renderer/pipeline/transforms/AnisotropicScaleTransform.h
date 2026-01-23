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

#ifndef POLAR_SHADER_TRANSFORMS_ANISOTROPICSCALETRANSFORM_H
#define POLAR_SHADER_TRANSFORMS_ANISOTROPICSCALETRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "../modulators/signals/ScalarSignals.h"

namespace PolarShader {
    /**
     * Scales Cartesian coordinates independently on X and Y using Q16.16 scale factors.
     * Inputs are bounded Q0.16 modulators mapped to an internal range.
     *
     * Parameters: sx, sy (BoundedScalarSignal, Q0.16).
     * Recommended order: early in Cartesian chain; combine with shear/tiling as needed before polar conversion.
     */
    class AnisotropicScaleTransform : public CartesianTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        AnisotropicScaleTransform(BoundedScalarSignal sx, BoundedScalarSignal sy);

        void advanceFrame(TimeMillis timeInMillis) override;

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //POLAR_SHADER_TRANSFORMS_ANISOTROPICSCALETRANSFORM_H
