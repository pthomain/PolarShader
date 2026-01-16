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

#include "TranslationTransform.h"
#include <cstring>
#include "polar/pipeline/utils/MathUtils.h"

namespace LEDSegments {
    struct TranslationTransform::State {
        LinearSignal dxSignal;
        LinearSignal dySignal;

        State(LinearSignal dx, LinearSignal dy)
            : dxSignal(std::move(dx)), dySignal(std::move(dy)) {}
    };

    TranslationTransform::TranslationTransform(LinearSignal dx, LinearSignal dy)
        : state(std::make_shared<State>(std::move(dx), std::move(dy))) {
    }

    void TranslationTransform::advanceFrame(Units::TimeMillis timeInMillis) {
        state->dxSignal.advanceFrame(timeInMillis);
        state->dySignal.advanceFrame(timeInMillis);
    }

    CartesianLayer TranslationTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y) {
            int32_t dx = state->dxSignal.getValue();
            int32_t dy = state->dySignal.getValue();

            uint32_t wrappedX = static_cast<uint32_t>(static_cast<int64_t>(x) + dx);
            uint32_t wrappedY = static_cast<uint32_t>(static_cast<int64_t>(y) + dy);
            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));
            return layer(finalX, finalY);
        };
    }
}
