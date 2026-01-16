//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#ifndef LED_SEGMENTS_TRANSFORMS_SHEARTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_SHEARTRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "polar/engine/pipeline/signals/Signal.h"
#include "polar/engine/pipeline/utils/MathUtils.h"

namespace LEDSegments {
    /**
     * Applies a shear (skew) to Cartesian coordinates: x' = x + kx*y, y' = y + ky*x.
     * Shear coefficients are LinearSignals (Q16.16), allowing animation. Wrap is explicit
     * modulo 2^32 to mirror other domain warps.
     *
     * Parameters: kx, ky (LinearSignals, Q16.16).
     * Recommended order: early in the Cartesian chain, before other warps/tiling, so downstream transforms
     * see the skewed space.
     */
    class ShearTransform : public CartesianTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        ShearTransform(LinearSignal kx, LinearSignal ky);

        void advanceFrame(Units::TimeMillis timeInMillis) override;

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_SHEARTRANSFORM_H
