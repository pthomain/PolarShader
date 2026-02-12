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

#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/transforms/base/Transforms.h"
#include "renderer/pipeline/signals/ranges/LinearRange.h"
#include <memory>

namespace PolarShader {
    /**
     * Cartesian domain warp driven by animated noise.
     *
     * - phaseSpeed: turns-per-second for the time axis (independent from scene duration).
     * - amplitude: signed signal mapped via unsigned range to scale maxOffset.
     * - warpScale: signed signal mapped via unsigned range, applied before sampling warp noise.
     * - maxOffset: signed signal mapped via unsigned range, maximum warp displacement in Q24.8 units.
     */
    class DomainWarpTransform : public UVTransform {
    public:
        enum class WarpType {
            Basic,
            FBM,
            Nested,
            Curl,
            Polar,
            Directional
        };

        DomainWarpTransform(
            SQ0_16Signal speed,
            SQ0_16Signal amplitude,
            SQ0_16Signal warpScale,
            SQ0_16Signal maxOffset,
            LinearRange<SQ24_8> warpScaleRange,
            LinearRange<SQ24_8> maxOffsetRange
        );

        DomainWarpTransform(
            WarpType type,
            SQ0_16Signal speed,
            SQ0_16Signal amplitude,
            SQ0_16Signal warpScale,
            SQ0_16Signal maxOffset,
            LinearRange<SQ24_8> warpScaleRange,
            LinearRange<SQ24_8> maxOffsetRange,
            uint8_t octaves,
            SQ0_16Signal flowDirection = SQ0_16Signal(),
            SQ0_16Signal flowStrength = SQ0_16Signal()
        );

        void advanceFrame(UQ0_16 progress, TimeMillis elapsedMs) override;

        UVMap operator()(const UVMap &layer) const override;

    private:
        struct MappedInputs;
        struct State;
        std::shared_ptr<State> state;

        static MappedInputs makeInputs(
            SQ0_16Signal speed,
            SQ0_16Signal amplitude,
            SQ0_16Signal warpScale,
            SQ0_16Signal maxOffset,
            LinearRange<SQ24_8> warpScaleRange,
            LinearRange<SQ24_8> maxOffsetRange,
            SQ0_16Signal flowDirection = SQ0_16Signal(),
            SQ0_16Signal flowStrength = SQ0_16Signal()
        );
    };
}

#endif // POLAR_SHADER_TRANSFORMS_DOMAINWARPTRANSFORM_H
