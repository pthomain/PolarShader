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

#ifndef POLAR_SHADER_TRANSFORMS_TILINGTRANSFORM_H
#define POLAR_SHADER_TRANSFORMS_TILINGTRANSFORM_H

#include "renderer/pipeline/maths/TilingMaths.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/transforms/base/Transforms.h"
#include <memory>

namespace PolarShader {
    /**
     * Tiles the cartesian plane into shape-aware cells and optionally mirrors every other cell.
     */
    class TilingTransform : public UVTransform {
    public:
        using TileShape = TilingMaths::TileShape;

        explicit TilingTransform(uint32_t cellSizeQ24_8, bool mirrored = false, TileShape shape = TileShape::SQUARE);

        explicit TilingTransform(
            Sf16Signal cellSize,
            bool mirrored = false,
            TileShape shape = TileShape::SQUARE
        );

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;

        int32_t getCellSizeRaw() const;

        UVMap operator()(const UVMap &layer) const override;

    private:
        struct State;
        std::shared_ptr<State> state;
    };
}

#endif // POLAR_SHADER_TRANSFORMS_TILINGTRANSFORM_H
