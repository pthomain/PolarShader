//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#ifndef LED_SEGMENTS_TRANSFORMS_LENSDISTORTIONTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_LENSDISTORTIONTRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "polar/engine/pipeline/signals/Signal.h"
#include "polar/engine/pipeline/utils/Units.h"

namespace LEDSegments {
    /**
     * Barrel/pincushion lens distortion on polar radius: r' = r * (1 + k * r), clamped to [0..FRACT_MAX].
     * Positive k produces barrel (expands outer radii), negative k produces pincushion (pulls in outer radii).
     *
     * Parameters: k (LinearSignal, Q16.16).
     * Recommended order: in polar chain before kaleidoscope/mandala so symmetry sees the distorted radius.
     */
    class LensDistortionTransform : public PolarTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        explicit LensDistortionTransform(LinearSignal k);

        void advanceFrame(Units::TimeMillis timeInMillis) override;

        PolarLayer operator()(const PolarLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_LENSDISTORTIONTRANSFORM_H
