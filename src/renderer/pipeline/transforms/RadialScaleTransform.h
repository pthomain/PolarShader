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

#ifndef POLAR_SHADER_TRANSFORMS_RADIALSCALETRANSFORM_H
#define POLAR_SHADER_TRANSFORMS_RADIALSCALETRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "../modulators/signals/AngularSignals.h"

namespace PolarShader {
    /**
     * Scales polar radius by a radial function: r' = r + (k * r), k in Q16.16.
     * Useful for tunnel/zoom effects. Output radius is clamped to [0, FRACT_Q0_16_MAX].
     *
     * Parameters: k (BoundedAngleSignal, Q0.16) mapped to an internal range.
     * Recommended order: in polar chain before kaleidoscope/mandala; combines well with vortex/rotation.
     */
    class RadialScaleTransform : public PolarTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        explicit RadialScaleTransform(BoundedAngleSignal k);

        void advanceFrame(TimeMillis timeInMillis) override;

        PolarLayer operator()(const PolarLayer &layer) const override;
    };
}

#endif //POLAR_SHADER_TRANSFORMS_RADIALSCALETRANSFORM_H
