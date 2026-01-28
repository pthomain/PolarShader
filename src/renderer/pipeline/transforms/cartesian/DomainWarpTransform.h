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

#ifndef POLAR_SHADER_TRANSFORMS_DOMAINWARPTRANSFORM_H
#define POLAR_SHADER_TRANSFORMS_DOMAINWARPTRANSFORM_H

#include "renderer/pipeline/signals/Accumulators.h"
#include "renderer/pipeline/transforms/base/Transforms.h"
#include "renderer/pipeline/units/CartesianUnits.h"
#include <memory>

namespace PolarShader {
    /**
     * Cartesian domain warp driven by animated noise.
     *
     * - phaseVelocity: turns-per-second for the time axis.
     * - amplitude: 0..1 signal that scales maxOffset.
     * - warpScale: Q24.8 scale applied to input coords before sampling warp noise.
     * - maxOffset: maximum warp displacement in Q24.8 units.
     */
    class DomainWarpTransform : public CartesianTransform {
    public:
        enum class WarpType {
            Basic,
            FBM,
            Nested,
            Curl,
            Polar,
            Directional
        };

    private:
        struct State;
        std::shared_ptr<State> state;

    public:
        DomainWarpTransform(
            SFracQ0_16Signal phaseVelocity,
            SFracQ0_16Signal amplitude,
            CartQ24_8 warpScale,
            CartQ24_8 maxOffset
        );

        DomainWarpTransform(
            WarpType type,
            SFracQ0_16Signal phaseVelocity,
            SFracQ0_16Signal amplitude,
            CartQ24_8 warpScale,
            CartQ24_8 maxOffset,
            uint8_t octaves,
            SFracQ0_16Signal flowDirection,
            SFracQ0_16Signal flowStrength
        );

        void advanceFrame(TimeMillis timeInMillis) override;

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif // POLAR_SHADER_TRANSFORMS_DOMAINWARPTRANSFORM_H
