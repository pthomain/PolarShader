//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#ifndef LED_SEGMENTS_TRANSFORMS_TIMEQUANTIZETRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_TIMEQUANTIZETRANSFORM_H

#include "base/Transforms.h"
#include "polar/engine/pipeline/utils/Units.h"

namespace LEDSegments {
    /**
     * Quantizes time passed to a PolarLayer to fixed steps (ms), producing stutter/stop-motion effects.
     *
     * Parameters: stepMillis (uint16_t).
     * Recommended order: late in polar chain, right before palette sampling, to affect all upstream motion.
     */
    class TimeQuantizeTransform : public PolarTransform {
        uint16_t stepMillis;

    public:
        explicit TimeQuantizeTransform(uint16_t stepMillis);

        PolarLayer operator()(const PolarLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_TIMEQUANTIZETRANSFORM_H
