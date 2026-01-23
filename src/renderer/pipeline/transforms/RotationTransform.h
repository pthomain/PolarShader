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

#ifndef POLAR_SHADER_TRANSFORMS_ROTATIONTRANSFORM_H
#define POLAR_SHADER_TRANSFORMS_ROTATIONTRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "renderer/pipeline/modulators/angular/BoundedAngularModulator.h"

namespace PolarShader {
    /**
     * @class RotationTransform
     * @brief Applies a dynamic rotation to the polar coordinates.
     *
     * Expects and returns PolarLayer phases in AngleTurnsUQ16_16 (AngleUnitsQ0_16 in high 16 bits).
     * Rotation signal is added full-resolution and wraps modulo 2^32. Negative velocities wrap via
     * unsigned addition.
     * Parameters: BoundedAngularModulator rotation.
     * Recommended order: after Cartesian → Polar conversion and before kaleidoscope/mandala folding
     * so symmetry uses the rotated frame.
     */
    class RotationTransform : public PolarTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        /**
         * @brief Constructs a new RotationTransform.
         * @param rotation A signal that provides the rotation angle over time.
         */
        explicit RotationTransform(BoundedAngularModulator rotation);

        void advanceFrame(TimeMillis timeInMillis) override;

        PolarLayer operator()(const PolarLayer &layer) const override;
    };
}

#endif //POLAR_SHADER_TRANSFORMS_ROTATIONTRANSFORM_H
