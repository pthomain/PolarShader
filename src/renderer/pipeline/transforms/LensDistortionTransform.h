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

#ifndef POLAR_SHADER_TRANSFORMS_LENSDISTORTIONTRANSFORM_H
#define POLAR_SHADER_TRANSFORMS_LENSDISTORTIONTRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "renderer/pipeline/signals/motion/scalar/ScalarMotion.h"
#include "renderer/pipeline/utils/Units.h"

namespace PolarShader {
    /**
     * Barrel/pincushion lens distortion on polar radius: r' = r * (1 + k * r), clamped to [0..FRACT_MAX].
     * Positive k produces barrel (expands outer radii), negative k produces pincushion (pulls in outer radii).
     *
     * Parameters: k (ScalarMotion, Q16.16).
     * Recommended order: in polar chain before kaleidoscope/mandala so symmetry sees the distorted radius.
     */
    class LensDistortionTransform : public PolarTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        explicit LensDistortionTransform(ScalarMotion k);

        void advanceFrame(TimeMillis timeInMillis) override;

        PolarLayer operator()(const PolarLayer &layer) const override;
    };
}

#endif //POLAR_SHADER_TRANSFORMS_LENSDISTORTIONTRANSFORM_H
