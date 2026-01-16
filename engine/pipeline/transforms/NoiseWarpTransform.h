//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#ifndef LED_SEGMENTS_TRANSFORMS_NOISEWARPTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_NOISEWARPTRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "polar/engine/pipeline/signals/Signal.h"

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
