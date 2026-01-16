//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#ifndef LED_SEGMENTS_TRANSFORMS_PERSPECTIVEWARPTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_PERSPECTIVEWARPTRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "polar/engine/pipeline/signals/Signal.h"
#include "polar/engine/pipeline/utils/MathUtils.h"

namespace LEDSegments {
    /**
     * Applies a simple perspective warp: scales coordinates by a factor based on Y (vanishing point).
     * x' = x * scale, y' = y * scale where scale = 1 / (1 + k * y). Clamp to avoid divide-by-zero.
     *
     * Parameters: k (LinearSignal, Q16.16). Positive k pulls far Y inward; negative pushes outward.
     * Recommended order: after base Cartesian warps (shear/scale), before tiling/mirroring or polar conversion.
     */
    class PerspectiveWarpTransform : public CartesianTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        explicit PerspectiveWarpTransform(LinearSignal k);

        void advanceFrame(Units::TimeMillis timeInMillis) override;

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_PERSPECTIVEWARPTRANSFORM_H
