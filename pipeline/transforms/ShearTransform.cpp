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

#include "ShearTransform.h"
#include <cstring>

namespace LEDSegments {
    struct ShearTransform::State {
        LinearSignal kxSignal;
        LinearSignal kySignal;

        State(LinearSignal kx, LinearSignal ky)
            : kxSignal(std::move(kx)), kySignal(std::move(ky)) {
        }
    };

    ShearTransform::ShearTransform(LinearSignal kx, LinearSignal ky)
        : state(std::make_shared<State>(std::move(kx), std::move(ky))) {
    }

    void ShearTransform::advanceFrame(Units::TimeMillis timeInMillis) {
        state->kxSignal.advanceFrame(timeInMillis);
        state->kySignal.advanceFrame(timeInMillis);
    }

    CartesianLayer ShearTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y) {
            Units::RawSignalQ16_16 kx_raw = state->kxSignal.getRawValue();
            Units::RawSignalQ16_16 ky_raw = state->kySignal.getRawValue();

            int64_t dx = static_cast<int64_t>(y) * kx_raw;
            int64_t dy = static_cast<int64_t>(x) * ky_raw;

            uint32_t wrappedX = static_cast<uint32_t>(static_cast<int64_t>(x) + (dx >> 16));
            uint32_t wrappedY = static_cast<uint32_t>(static_cast<int64_t>(y) + (dy >> 16));

            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));

            return layer(finalX, finalY);
        };
    }
}
