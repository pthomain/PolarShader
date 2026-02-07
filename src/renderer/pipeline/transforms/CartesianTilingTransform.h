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

#ifndef POLAR_SHADER_TRANSFORMS_CARTESIAN_CARTESIANTILINGTRANSFORM_H
#define POLAR_SHADER_TRANSFORMS_CARTESIAN_CARTESIANTILINGTRANSFORM_H

#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/transforms/base/Transforms.h"
#include <memory>

namespace PolarShader {
    /**
     * Tiles the cartesian plane into square cells of uniform size and optionally mirrors every other cell.
     */
    class CartesianTilingTransform : public UVTransform {
        struct State;
        std::shared_ptr<State> state;

        struct MappedInputs {
            MappedSignal<int32_t> cellSizeSignal;
        };

        CartesianTilingTransform(MappedInputs inputs, bool mirrored);

        static MappedInputs makeInputs(
            SFracQ0_16Signal cellSize,
            int32_t minCellSize,
            int32_t maxCellSize
        );

    public:
        explicit CartesianTilingTransform(uint32_t cellSizeQ24_8, bool mirrored = false);

        CartesianTilingTransform(
            SFracQ0_16Signal cellSize,
            int32_t minCellSize = 4096,
            int32_t maxCellSize = 65536,
            bool mirrored = false
        );

        void advanceFrame(TimeMillis timeInMillis) override;

        UVMap operator()(const UVMap &layer) const override;
    };
}

#endif // POLAR_SHADER_TRANSFORMS_CARTESIAN_CARTESIANTILINGTRANSFORM_H
