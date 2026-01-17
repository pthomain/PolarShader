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

#include "AnisotropicScaleTransform.h"
#include <cstring>

namespace LEDSegments {
    struct AnisotropicScaleTransform::State {
        ScalarMotion sxSignal;
        ScalarMotion sySignal;

        State(ScalarMotion sx, ScalarMotion sy)
            : sxSignal(std::move(sx)), sySignal(std::move(sy)) {
        }
    };

    AnisotropicScaleTransform::AnisotropicScaleTransform(ScalarMotion sx, ScalarMotion sy)
        : state(std::make_shared<State>(std::move(sx), std::move(sy))) {
    }

    void AnisotropicScaleTransform::advanceFrame(TimeMillis timeInMillis) {
        state->sxSignal.advanceFrame(timeInMillis);
        state->sySignal.advanceFrame(timeInMillis);
    }

    CartesianLayer AnisotropicScaleTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y) {
            RawQ16_16 sx_raw = state->sxSignal.getRawValue();
            RawQ16_16 sy_raw = state->sySignal.getRawValue();

            int64_t scaledX = static_cast<int64_t>(x) * raw(sx_raw);
            int64_t scaledY = static_cast<int64_t>(y) * raw(sy_raw);

            uint32_t wrappedX = static_cast<uint32_t>(scaledX >> 16);
            uint32_t wrappedY = static_cast<uint32_t>(scaledY >> 16);

            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));

            return layer(finalX, finalY);
        };
    }
}
