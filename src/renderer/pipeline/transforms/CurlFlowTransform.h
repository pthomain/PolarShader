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

#ifndef POLAR_SHADER_TRANSFORMS_CURLFLOWTRANSFORM_H
#define POLAR_SHADER_TRANSFORMS_CURLFLOWTRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "../modulators/signals/ScalarSignals.h"
#include "renderer/pipeline/utils/Units.h"

namespace PolarShader {
    /**
     * Divergence-free (curl) flow advection: approximates a curl of a low-frequency noise field and
     * offsets Cartesian coordinates accordingly. Useful for fluid-like motion with stable density.
     *
     * Parameters: amplitude (BoundedScalarSignal, Q0.16) mapped to an internal range,
     * sampleShift (uint8, power-of-two divisor for sampling).
     * Recommended order: early in Cartesian chain, before other warps/tiling, so downstream transforms
     * inherit the flow.
     */
    class CurlFlowTransform : public CartesianTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        CurlFlowTransform(BoundedScalarSignal amplitude, uint8_t sampleShift = 3);

        void advanceFrame(TimeMillis timeInMillis) override;

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //POLAR_SHADER_TRANSFORMS_CURLFLOWTRANSFORM_H
