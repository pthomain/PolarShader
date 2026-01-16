//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

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

#include "ZoomTransform.h"
#include <cstring>

namespace LEDSegments {
    struct ZoomTransform::State {
        LinearSignal scaleSignal;

        explicit State(LinearSignal s) : scaleSignal(std::move(s)) {
        }
    };

    ZoomTransform::ZoomTransform(LinearSignal scale)
        : state(std::make_shared<State>(std::move(scale))) {
    }

    void ZoomTransform::advanceFrame(Units::TimeMillis timeInMillis) {
        state->scaleSignal.advanceFrame(timeInMillis);
    }

    CartesianLayer ZoomTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y) {
            Units::RawSignalQ16_16 s_raw = state->scaleSignal.getRawValue();
            // Allow scale == 0 to collapse to origin; negative scales flip axes.
            int64_t sx = static_cast<int64_t>(x) * s_raw;
            int64_t sy = static_cast<int64_t>(y) * s_raw;

            uint32_t wrappedX = static_cast<uint32_t>(sx >> 16);
            uint32_t wrappedY = static_cast<uint32_t>(sy >> 16);

            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));
            return layer(finalX, finalY);
        };
    }
}
