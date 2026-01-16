//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#ifndef LED_SEGMENTS_TRANSFORMS_TILEJITTERTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_TILEJITTERTRANSFORM_H

#include "base/Transforms.h"
#include "polar/engine/pipeline/utils/Units.h"
#include <memory>

namespace LEDSegments {
    /**
     * Adds a small, per-tile offset after tiling to break repetition. Uses tile index noise to jitter.
     *
     * Parameters: tileX, tileY (tile sizes), amplitude (uint16_t jitter magnitude in coordinate units).
     * Recommended order: immediately after TilingTransform in the Cartesian chain.
     */
    class TileJitterTransform : public CartesianTransform {
        uint32_t tileX;
        uint32_t tileY;
        uint16_t amplitude;

    public:
        TileJitterTransform(uint32_t tileX, uint32_t tileY, uint16_t amplitude);

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_TILEJITTERTRANSFORM_H
