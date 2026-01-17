//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2023 Pierre Thomain

/*
 * This file is part of LED Segments.
 *
 * LED Segments is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LED Segments is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LED Segments. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LED_SEGMENTS_TRANSFORMS_ZOOMTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_ZOOMTRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "polar/pipeline/signals/motion/Motion.h"

namespace LEDSegments {
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

#endif //LED_SEGMENTS_TRANSFORMS_ZOOMTRANSFORM_H
