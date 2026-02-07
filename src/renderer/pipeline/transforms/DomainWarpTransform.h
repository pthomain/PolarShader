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

#include "renderer/pipeline/transforms/base/Transforms.h"
#include "renderer/pipeline/ranges/LinearRange.h"
#include <memory>

namespace PolarShader {
    /**
     * Cartesian domain warp driven by animated noise.
     *
     * - phaseSpeed: turns-per-second for the time axis.
     * - amplitude: 0..1 signal that scales maxOffset.
     * - warpScale: 0..1 signal mapped via LinearRange, applied before sampling warp noise.
     * - maxOffset: 0..1 signal mapped via LinearRange, maximum warp displacement in Q24.8 units.
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

    private:
        struct MappedInputs;
        struct State;
        std::shared_ptr<State> state;

        DomainWarpTransform(
            WarpType type,
            MappedSignal<SFracQ0_16> phaseSpeed,
            MappedSignal<int32_t> amplitude,
            MappedSignal<CartQ24_8> warpScale,
            std::shared_ptr<MappedSignal<CartQ24_8>> maxOffset,
            uint8_t octaves,
            MappedSignal<FracQ0_16> flowDirection,
            MappedSignal<int32_t> flowStrength
        );

        DomainWarpTransform(
            WarpType type,
            uint8_t octaves,
            MappedInputs inputs
        );

        static MappedInputs makeInputs(
            SFracQ0_16Signal phaseSpeed,
            SFracQ0_16Signal amplitude,
            SFracQ0_16Signal warpScale,
            SFracQ0_16Signal maxOffset,
            LinearRange<CartQ24_8> warpScaleRange,
            LinearRange<CartQ24_8> maxOffsetRange
        );

        static MappedInputs makeDirectionalInputs(
            SFracQ0_16Signal phaseSpeed,
            SFracQ0_16Signal amplitude,
            SFracQ0_16Signal warpScale,
            SFracQ0_16Signal maxOffset,
            LinearRange<CartQ24_8> warpScaleRange,
            LinearRange<CartQ24_8> maxOffsetRange,
            SFracQ0_16Signal flowDirection,
            SFracQ0_16Signal flowStrength
        );

        static MappedInputs makeInputsInternal(
            SFracQ0_16Signal phaseSpeed,
            SFracQ0_16Signal amplitude,
            SFracQ0_16Signal warpScale,
            SFracQ0_16Signal maxOffset,
            LinearRange<CartQ24_8> warpScaleRange,
            LinearRange<CartQ24_8> maxOffsetRange,
            SFracQ0_16Signal flowDirection,
            SFracQ0_16Signal flowStrength
        );

    public:
        DomainWarpTransform(
            SFracQ0_16Signal phaseSpeed,
            SFracQ0_16Signal amplitude,
            SFracQ0_16Signal warpScale,
            SFracQ0_16Signal maxOffset,
            LinearRange<CartQ24_8> warpScaleRange,
            LinearRange<CartQ24_8> maxOffsetRange
        );

        DomainWarpTransform(
            WarpType type,
            SFracQ0_16Signal phaseSpeed,
            SFracQ0_16Signal amplitude,
            SFracQ0_16Signal warpScale,
            SFracQ0_16Signal maxOffset,
            LinearRange<CartQ24_8> warpScaleRange,
            LinearRange<CartQ24_8> maxOffsetRange,
            uint8_t octaves,
            SFracQ0_16Signal flowDirection,
            SFracQ0_16Signal flowStrength
        );

        void advanceFrame(TimeMillis timeInMillis) override;

        UVMap operator()(const UVMap &layer) const override;
    };
}

#endif // POLAR_SHADER_TRANSFORMS_DOMAINWARPTRANSFORM_H
