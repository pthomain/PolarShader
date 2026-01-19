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

#include <memory>
#include "base/Transforms.h"
#include "renderer/pipeline/signals/motion/scalar/ScalarMotion.h"

namespace PolarShader {
    /**
     * Uniform Cartesian zoom: (x, y) -> (x * s, y * s), s in Q16.16.
     * Positive s > 1 zooms in (expands coordinates), 0..1 zooms out.
     *
     * Parameters: scale (ScalarMotion, Q16.16).
     * Recommended order: early in Cartesian chain before warps/tiling.
     */
    class ZoomTransform : public CartesianTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        // Standard bounds for safe zoom: [1/16x, 1x]
        inline static const FracQ16_16 ZOOM_MIN = FracQ16_16::fromRaw(Q16_16_ONE >> 4); // 0.0625x
        inline static const FracQ16_16 ZOOM_MAX = FracQ16_16::fromRaw(Q16_16_ONE); // 1.0x
        inline static const FracQ16_16 ZOOM_MID =
                FracQ16_16::fromRaw((ZOOM_MIN.asRaw() + ZOOM_MAX.asRaw()) / 2); // ~0.53125x
        inline static const FracQ16_16 ZOOM_SPAN =
                FracQ16_16::fromRaw(ZOOM_MAX.asRaw() - ZOOM_MIN.asRaw());
        inline static const FracQ16_16 ZOOM_HALF_SPAN =
                FracQ16_16::fromRaw(ZOOM_SPAN.asRaw() / 2);

        explicit ZoomTransform(ScalarMotion scale);

        ZoomTransform(FracQ16_16 base,
                      FracQ16_16 amplitude,
                      FracQ16_16 phaseVelocity,
                      AngleTurnsUQ16_16 initialPhase = AngleTurnsUQ16_16(0));

        void advanceFrame(TimeMillis timeInMillis) override;

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //POLAR_SHADER_TRANSFORMS_ZOOMTRANSFORM_H
