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

#include "CurlFlowTransform.h"
#include <cstring>
#include "FastLED.h"

namespace LEDSegments {
    struct CurlFlowTransform::State {
        LinearSignal amplitudeSignal;
        uint8_t sampleShift;

        State(LinearSignal amp, uint8_t shift)
            : amplitudeSignal(std::move(amp)), sampleShift(shift) {
        }
    };

    CurlFlowTransform::CurlFlowTransform(LinearSignal amplitude, uint8_t sampleShift)
        : state(std::make_shared<State>(std::move(amplitude), sampleShift)) {
    }

    void CurlFlowTransform::advanceFrame(Units::TimeMillis timeInMillis) {
        state->amplitudeSignal.advanceFrame(timeInMillis);
    }

    CartesianLayer CurlFlowTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y) {
            const uint8_t shift = state->sampleShift;
            const int32_t dx = 256;
            // Sample low-frequency noise for gradient
            int32_t n_x1 = static_cast<int32_t>(inoise16((x + dx) >> shift, y >> shift)) - Units::U16_HALF;
            int32_t n_x0 = static_cast<int32_t>(inoise16((x - dx) >> shift, y >> shift)) - Units::U16_HALF;
            int32_t n_y1 = static_cast<int32_t>(inoise16(x >> shift, (y + dx) >> shift)) - Units::U16_HALF;
            int32_t n_y0 = static_cast<int32_t>(inoise16(x >> shift, (y - dx) >> shift)) - Units::U16_HALF;

            int32_t grad_x = (n_x1 - n_x0); // approximate d/dx
            int32_t grad_y = (n_y1 - n_y0); // approximate d/dy

            // curl = (grad_y, -grad_x)
            Units::RawSignalQ16_16 amp = state->amplitudeSignal.getRawValue();
            // Gradients are large (~±65535); scale them down before applying amp to avoid extreme warps.
            constexpr uint8_t GRAD_SCALE_SHIFT = 8; // damp gradient magnitude by 256
            int64_t offset_x = (static_cast<int64_t>(grad_y) * amp) >> (16 + GRAD_SCALE_SHIFT);
            int64_t offset_y = (static_cast<int64_t>(-grad_x) * amp) >> (16 + GRAD_SCALE_SHIFT);

            uint32_t wrappedX = static_cast<uint32_t>(static_cast<int64_t>(x) + offset_x);
            uint32_t wrappedY = static_cast<uint32_t>(static_cast<int64_t>(y) + offset_y);

            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));

            return layer(finalX, finalY);
        };
    }
}
