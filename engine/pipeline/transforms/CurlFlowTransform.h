//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#ifndef LED_SEGMENTS_TRANSFORMS_CURLFLOWTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_CURLFLOWTRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "polar/engine/pipeline/signals/Signal.h"
#include "polar/engine/pipeline/utils/Units.h"

namespace LEDSegments {
    /**
     * Divergence-free (curl) flow advection: approximates a curl of a low-frequency noise field and
     * offsets Cartesian coordinates accordingly. Useful for fluid-like motion with stable density.
     *
     * Parameters: amplitude (LinearSignal, Q16.16), sampleShift (uint8, power-of-two divisor for sampling).
     * Recommended order: early in Cartesian chain, before other warps/tiling, so downstream transforms
     * inherit the flow.
     */
    class CurlFlowTransform : public CartesianTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        CurlFlowTransform(LinearSignal amplitude, uint8_t sampleShift = 3);

        void advanceFrame(Units::TimeMillis timeInMillis) override;

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_CURLFLOWTRANSFORM_H
