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

#ifndef POLAR_SHADER_TRANSFORMS_FLOWFIELDTRANSFORM_H
#define POLAR_SHADER_TRANSFORMS_FLOWFIELDTRANSFORM_H

#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"
#include "renderer/pipeline/transforms/base/Transforms.h"
#include <memory>

namespace PolarShader {
    /**
     * Noise-driven Cartesian vector field.
     *
     * Visual effect summary:
     * - Raising `phaseSpeed` animates the field faster.
     * - Raising `flowStrength` increases how strongly UVs follow the field.
     * - Raising `fieldScale` makes the field denser and more turbulent.
     * - Raising `maxOffset` allows larger UV displacements.
     */
    class FlowFieldTransform : public UVTransform {
    public:
        static constexpr int32_t DEFAULT_FIELD_SCALE_MIN_RAW = 1 << 8;  // 1.0 in Q24.8
        static constexpr int32_t DEFAULT_FIELD_SCALE_MAX_RAW = 3 << 8;  // 3.0 in Q24.8
        static constexpr int32_t DEFAULT_MAX_OFFSET_MIN_RAW = 1 << 5;   // 0.125 UV units
        static constexpr int32_t DEFAULT_MAX_OFFSET_MAX_RAW = 1 << 7;   // 0.5 UV units

        /**
         * @param phaseSpeed Signed phase velocity for the animated vector field, in turns/sec.
         * Negative values run the field animation backwards.
         * @param flowStrength Unsigned strength factor for the sampled vector field.
         * This is mapped through `magnitudeRange()` and multiplied by `maxOffset`.
         * @param fieldScale Unsigned control for the spatial density of the field.
         * Higher values produce tighter, more rapidly changing local directions.
         * @param maxOffset Unsigned cap for the UV displacement produced by the field.
         * @param fieldScaleRange Output range for `fieldScale`.
         * @param maxOffsetRange Output range for `maxOffset`.
         */
        FlowFieldTransform(
            Sf16Signal phaseSpeed = constant(650),
            Sf16Signal flowStrength = constant(250),
            Sf16Signal fieldScale = constant(0),
            Sf16Signal maxOffset = constant(0),
            MagnitudeRange<fl::s24x8> fieldScaleRange = MagnitudeRange(
                fl::s24x8::from_raw(DEFAULT_FIELD_SCALE_MIN_RAW),
                fl::s24x8::from_raw(DEFAULT_FIELD_SCALE_MAX_RAW)
            ),
            MagnitudeRange<fl::s24x8> maxOffsetRange = MagnitudeRange(
                fl::s24x8::from_raw(DEFAULT_MAX_OFFSET_MIN_RAW),
                fl::s24x8::from_raw(DEFAULT_MAX_OFFSET_MAX_RAW)
            )
        );

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;

        UVMap operator()(const UVMap &layer) const override;

    private:
        struct State;
        std::shared_ptr<State> state;
    };
}

#endif // POLAR_SHADER_TRANSFORMS_FLOWFIELDTRANSFORM_H
