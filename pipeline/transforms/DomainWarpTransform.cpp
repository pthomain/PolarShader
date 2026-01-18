//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2023 Pierre Thomain

/*
 * This file is part of LED Segments.
 *
 * LED Segments is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LED Segments is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LED Segments. If not, see <https://www.gnu.org/licenses/>.
 */

#include "DomainWarpTransform.h"
#include <cstring>

namespace LEDSegments {
    struct DomainWarpTransform::State {
        LinearMotion warpSignal;

        explicit State(LinearMotion warp)
            : warpSignal(std::move(warp)) {
        }
    };

    DomainWarpTransform::DomainWarpTransform(LinearMotion warp)
        : state(std::make_shared<State>(std::move(warp))) {
    }

    void DomainWarpTransform::advanceFrame(TimeMillis timeInMillis) {
        state->warpSignal.advanceFrame(timeInMillis);
    }

    CartesianLayer DomainWarpTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y) {
            // Use full Q16.16 offsets to preserve sub-pixel motion.
            RawQ16_16 warpX = state->warpSignal.getRawX();
            RawQ16_16 warpY = state->warpSignal.getRawY();

            // Add the original coordinate and the warp offset using 64-bit arithmetic
            // to prevent signed overflow, then perform a well-defined wrap to the 32-bit domain.
            uint32_t wrappedX = static_cast<uint32_t>(static_cast<int64_t>(x) + raw(warpX));
            uint32_t wrappedY = static_cast<uint32_t>(static_cast<int64_t>(y) + raw(warpY));
            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));

            return layer(finalX, finalY);
        };
    }
}
