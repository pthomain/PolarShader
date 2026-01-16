//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#include "TileJitterTransform.h"
#include <cstring>
#include "FastLED.h"

namespace LEDSegments {

    TileJitterTransform::TileJitterTransform(uint32_t tileX, uint32_t tileY, uint16_t amplitude)
        : tileX(tileX), tileY(tileY), amplitude(amplitude) {
    }

    CartesianLayer TileJitterTransform::operator()(const CartesianLayer &layer) const {
        return [tileX = this->tileX, tileY = this->tileY, amp = this->amplitude, layer](int32_t x, int32_t y, Units::TimeMillis t) {
            // Use floor division for signed coordinates to ensure continuous tile indices across 0.
            auto floor_div = [](int32_t val, uint32_t div) -> int32_t {
                if (div == 0) return 0;
                int32_t d = static_cast<int32_t>(div);
                return (val >= 0) ? (val / d) : ((val - d + 1) / d);
            };

            int32_t tileIdxX = floor_div(x, tileX);
            int32_t tileIdxY = floor_div(y, tileY);

            // Cast to unsigned for noise hashing (wraps naturally)
            uint16_t jitterX = (tileX != 0) ? inoise16(static_cast<uint32_t>(tileIdxX), static_cast<uint32_t>(tileIdxY)) : 0;
            uint16_t jitterY = (tileY != 0) ? inoise16(static_cast<uint32_t>(tileIdxY), static_cast<uint32_t>(tileIdxX)) : 0;

            int32_t signedJx = static_cast<int32_t>(jitterX) - Units::U16_HALF;
            int32_t signedJy = static_cast<int32_t>(jitterY) - Units::U16_HALF;

            int32_t offsetX = (signedJx * amp) >> 15; // scale to amplitude
            int32_t offsetY = (signedJy * amp) >> 15;

            uint32_t wrappedX = static_cast<uint32_t>(static_cast<int64_t>(x) + offsetX);
            uint32_t wrappedY = static_cast<uint32_t>(static_cast<int64_t>(y) + offsetY);

            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));

            return layer(finalX, finalY, t);
        };
    }
}
