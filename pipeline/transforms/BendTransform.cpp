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

#include "BendTransform.h"
#include <cstring>
#include <cstdlib>
#include <cstdint>

namespace LEDSegments {
    struct BendTransform::State {
        ScalarMotion kxSignal;
        ScalarMotion kySignal;

        State(ScalarMotion kx, ScalarMotion ky)
            : kxSignal(std::move(kx)), kySignal(std::move(ky)) {
        }
    };

    BendTransform::BendTransform(ScalarMotion kx, ScalarMotion ky)
        : state(std::make_shared<State>(std::move(kx), std::move(ky))) {
    }

    void BendTransform::advanceFrame(Units::TimeMillis timeInMillis) {
        state->kxSignal.advanceFrame(timeInMillis);
        state->kySignal.advanceFrame(timeInMillis);
    }

    CartesianLayer BendTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y) {
            Units::RawFracQ16_16 kx_raw = state->kxSignal.getRawValue();
            Units::RawFracQ16_16 ky_raw = state->kySignal.getRawValue();

            auto safe_bend = [](int32_t k_raw, int32_t coord) -> int64_t {
                if (k_raw == 0) return 0;
                int64_t squared = static_cast<int64_t>(coord) * static_cast<int64_t>(coord);
                int64_t abs_k = std::llabs(static_cast<int64_t>(k_raw));
                int64_t limit = (abs_k == 0) ? 0 : (INT64_MAX / abs_k);
                if (squared > limit) squared = limit;
                return (squared * k_raw) >> 16;
            };

            int64_t bendX = safe_bend(kx_raw, y);
            int64_t bendY = safe_bend(ky_raw, x);

            uint32_t wrappedX = static_cast<uint32_t>(static_cast<int64_t>(x) + bendX);
            uint32_t wrappedY = static_cast<uint32_t>(static_cast<int64_t>(y) + bendY);

            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));

            return layer(finalX, finalY);
        };
    }
}
