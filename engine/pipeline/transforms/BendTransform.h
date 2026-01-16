//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#ifndef LED_SEGMENTS_TRANSFORMS_BENDTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_BENDTRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "polar/engine/pipeline/signals/Signal.h"
#include "polar/engine/pipeline/utils/MathUtils.h"

namespace LEDSegments {
    /**
     * Applies a simple bend/curve to Cartesian coordinates: x' = x + k * y^2 (and/or y' = y + k' * x^2).
     * Bend coefficients are LinearSignals (Q16.16). Inputs are clamped to int32; wraps modulo 2^32.
     *
     * Parameters: kx, ky (LinearSignals, Q16.16).
     * Recommended order: after basic scale/shear, before polar conversion, to impose curvature on the domain.
     */
    class BendTransform : public CartesianTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        BendTransform(LinearSignal kx, LinearSignal ky);

        void advanceFrame(Units::TimeMillis timeInMillis) override;

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_BENDTRANSFORM_H
