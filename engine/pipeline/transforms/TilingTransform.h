//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#ifndef LED_SEGMENTS_TRANSFORMS_TILINGTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_TILINGTRANSFORM_H

#include <memory>
#include "base/Transforms.h"

namespace LEDSegments {
    /**
     * Tiles Cartesian space by wrapping coordinates into a tile of size (tileX, tileY)
     * using signed modulo; negative coordinates wrap back into [0, tileN).
     * Useful for repeating patterns. A tile dimension of 0 leaves that axis unchanged.
     *
     * Parameters: tileX, tileY (unsigned tile sizes in coordinate units).
     * Recommended order: early in Cartesian chain; combine with shear/scale before polar conversion.
     */
    class TilingTransform : public CartesianTransform {
        uint32_t tileX;
        uint32_t tileY;

    public:
        TilingTransform(uint32_t tileX, uint32_t tileY);

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_TILINGTRANSFORM_H
