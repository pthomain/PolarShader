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

#ifndef POLAR_SHADER_TRANSFORMS_ZOOMTRANSFORM_H
#define POLAR_SHADER_TRANSFORMS_ZOOMTRANSFORM_H

#include <renderer/pipeline/signals/SignalTypes.h>
#include <renderer/pipeline/transforms/base/Transforms.h>

namespace PolarShader {
    /**
     * Uniform Cartesian zoom: (x, y) -> (x * s, y * s), s in f16/sf16.
     * s in 0..~1 scales towards the origin; negative values flip axes.
     *
     * Parameters: scale (Sf16Signal, f16/sf16), mapped to the transform's zoom range.
     * Recommended order: early in Cartesian chain before warps/tiling.
     */
    class ZoomTransform : public UVTransform {
    public:
        explicit ZoomTransform(Sf16Signal scale);

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;

        UVMap operator()(const UVMap &layer) const override;

    private:
        struct MappedInputs;
        static MappedInputs makeInputs(Sf16Signal scale);

        struct State;
        std::shared_ptr<State> state;
    };
}

#endif //POLAR_SHADER_TRANSFORMS_ZOOMTRANSFORM_H
