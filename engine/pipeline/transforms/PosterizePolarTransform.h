//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#ifndef LED_SEGMENTS_TRANSFORMS_POSTERIZEPOLARTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_POSTERIZEPOLARTRANSFORM_H

#include "base/Transforms.h"
#include "polar/engine/pipeline/utils/Units.h"

namespace LEDSegments {
    /**
     * Quantizes polar coordinates into discrete bands: angle and/or radius are snapped to bins.
     * Useful for posterized rings/sectors before applying vortex/rotation.
     *
     * Parameters: angleBins (1..255, 0/1 disables), radiusBins (1..255, 0/1 disables).
     * Recommended order: early in polar chain, before vortex/kaleidoscope.
     */
    class PosterizePolarTransform : public PolarTransform {
        uint8_t angleBins;
        uint8_t radiusBins;

    public:
        PosterizePolarTransform(uint8_t angleBins, uint8_t radiusBins);

        PolarLayer operator()(const PolarLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_POSTERIZEPOLARTRANSFORM_H
