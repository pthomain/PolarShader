//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#ifndef LED_SEGMENTS_TRANSFORMS_MIRRORTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_MIRRORTRANSFORM_H

#include "base/Transforms.h"

namespace LEDSegments {
    /**
     * Mirrors Cartesian coordinates across X and/or Y axes by taking absolute value.
     * Useful for symmetry without changing topology.
     *
     * Parameters: mirrorX, mirrorY (booleans).
     * Recommended order: after tiling/scale/shear in Cartesian space; before polar conversion so symmetry
     * propagates through downstream transforms.
     */
    class MirrorTransform : public CartesianTransform {
        bool mirrorX;
        bool mirrorY;

    public:
        MirrorTransform(bool mirrorX, bool mirrorY);

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_MIRRORTRANSFORM_H
