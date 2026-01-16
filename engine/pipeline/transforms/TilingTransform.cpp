//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#include "TilingTransform.h"
#include <cstring>

namespace LEDSegments {

    TilingTransform::TilingTransform(uint32_t tileX, uint32_t tileY)
        : tileX(tileX), tileY(tileY) {
    }

    CartesianLayer TilingTransform::operator()(const CartesianLayer &layer) const {
        return [tileX = this->tileX, tileY = this->tileY, layer](int32_t x, int32_t y, Units::TimeMillis t) {
            auto tile_mod = [](int32_t v, uint32_t tile) -> int32_t {
                if (tile == 0) return v;
                int32_t m = v % static_cast<int32_t>(tile);
                if (m < 0) m += static_cast<int32_t>(tile);
                return m;
            };

            int32_t finalX = tile_mod(x, tileX);
            int32_t finalY = tile_mod(y, tileY);

            return layer(finalX, finalY, t);
        };
    }
}
