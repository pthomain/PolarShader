//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#ifndef LED_SEGMENTS_TRANSFORMS_RADIALSCALETRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_RADIALSCALETRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "polar/engine/pipeline/signals/Signal.h"
#include "polar/engine/pipeline/utils/MathUtils.h"

namespace LEDSegments {
    /**
     * Scales polar radius by a radial function: r' = r + (k * r) where k is a LinearSignal (Q16.16).
     * Useful for tunnel/zoom effects. Output radius is clamped to [0, FRACT_Q0_16_MAX].
     *
     * Parameters: k (LinearSignal, Q16.16 factor).
     * Recommended order: in polar chain before kaleidoscope/mandala; combines well with vortex/rotation.
     */
    class RadialScaleTransform : public PolarTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        explicit RadialScaleTransform(LinearSignal k);

        void advanceFrame(Units::TimeMillis timeInMillis) override;

        PolarLayer operator()(const PolarLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_RADIALSCALETRANSFORM_H
