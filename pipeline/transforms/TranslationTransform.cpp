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

#include "TranslationTransform.h"
#include <cstring>

namespace LEDSegments {
    struct TranslationTransform::State {
        LinearMotion offsetSignal;

        explicit State(LinearMotion offset)
            : offsetSignal(std::move(offset)) {}
    };

    TranslationTransform::TranslationTransform(LinearMotion offset)
        : state(std::make_shared<State>(std::move(offset))) {
    }

    void TranslationTransform::advanceFrame(TimeMillis timeInMillis) {
        state->offsetSignal.advanceFrame(timeInMillis);
    }

    CartesianLayer TranslationTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y) {
            RawQ16_16 dx_raw = state->offsetSignal.getRawX();
            RawQ16_16 dy_raw = state->offsetSignal.getRawY();

            uint32_t wrappedX = static_cast<uint32_t>(static_cast<int64_t>(x) + raw(dx_raw));
            uint32_t wrappedY = static_cast<uint32_t>(static_cast<int64_t>(y) + raw(dy_raw));
            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));
            return layer(finalX, finalY);
        };
    }
}
