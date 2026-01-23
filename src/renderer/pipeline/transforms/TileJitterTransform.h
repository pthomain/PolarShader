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

#ifndef POLAR_SHADER_TRANSFORMS_TILEJITTERTRANSFORM_H
#define POLAR_SHADER_TRANSFORMS_TILEJITTERTRANSFORM_H

#include "base/Transforms.h"
#include "renderer/pipeline/utils/Units.h"
#include "../modulators/signals/ScalarSignals.h"

namespace PolarShader {
    /**
     * Adds a small, per-tile offset after tiling to break repetition. Uses tile index noise to jitter.
     *
     * Parameters: tileX, tileY (tile sizes), amplitude (BoundedScalarSignal, Q0.16) mapped to
     * an internal range in coordinate units.
     * Recommended order: immediately after TilingTransform in the Cartesian chain.
     */
    class TileJitterTransform : public CartesianTransform {
        uint32_t tileX;
        uint32_t tileY;
        BoundedScalarSignal amplitudeSignal;
        UnboundedScalar amplitudeValue = UnboundedScalar(0);

    public:
        TileJitterTransform(uint32_t tileX, uint32_t tileY, BoundedScalarSignal amplitude);

        void advanceFrame(TimeMillis timeInMillis) override;
        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //POLAR_SHADER_TRANSFORMS_TILEJITTERTRANSFORM_H
