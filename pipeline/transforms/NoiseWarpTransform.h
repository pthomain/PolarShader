//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

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

#ifndef LED_SEGMENTS_TRANSFORMS_NOISEWARPTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_NOISEWARPTRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "polar/pipeline/signals/Signal.h"

namespace LEDSegments {
    /**
     * Noise-weighted warp of Cartesian coordinates: x' = x + noise(x,y)*kx, y' = y + noise(x,y)*ky.
     * Uses raw inoise16 deviation around midpoint; coefficients are LinearSignals (Q16.16).
     * Useful for smoke/liquid-like distortion. Wrap is explicit modulo 2^32.
     *
     * Parameters: kx, ky (LinearSignals, Q16.16).
     * Recommended order: apply to source coordinates before sampling noise or tiling to avoid self-warp feedback.
     */
    class NoiseWarpTransform : public CartesianTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        NoiseWarpTransform(LinearSignal kx, LinearSignal ky);

        void advanceFrame(Units::TimeMillis timeInMillis) override;

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_NOISEWARPTRANSFORM_H
