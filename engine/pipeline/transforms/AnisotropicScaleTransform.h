//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#ifndef LED_SEGMENTS_TRANSFORMS_ANISOTROPICSCALETRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_ANISOTROPICSCALETRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "polar/engine/pipeline/signals/Signal.h"
#include "polar/engine/pipeline/utils/MathUtils.h"

namespace LEDSegments {
    /**
     * Scales Cartesian coordinates independently on X and Y using Q16.16 scale factors
     * (LinearSignals). Wrap is modulo 2^32 to match other domain warps.
     *
     * Parameters: sx, sy (LinearSignals, Q16.16).
     * Recommended order: early in Cartesian chain; combine with shear/tiling as needed before polar conversion.
     */
    class AnisotropicScaleTransform : public CartesianTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        AnisotropicScaleTransform(LinearSignal sx, LinearSignal sy);

        void advanceFrame(Units::TimeMillis timeInMillis) override;

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_ANISOTROPICSCALETRANSFORM_H
