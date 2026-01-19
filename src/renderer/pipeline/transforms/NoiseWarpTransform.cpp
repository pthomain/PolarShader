//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

/*
 * This file is part of PolarShader.
 *
 * PolarShader is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PolarShader is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PolarShader. If not, see <https://www.gnu.org/licenses/>.
 */

#include "NoiseWarpTransform.h"
#include <cstring>

namespace PolarShader {
    struct NoiseWarpTransform::State {
        ScalarMotion kxSignal;
        ScalarMotion kySignal;

        State(ScalarMotion kx, ScalarMotion ky)
            : kxSignal(std::move(kx)), kySignal(std::move(ky)) {
        }
    };

    NoiseWarpTransform::NoiseWarpTransform(ScalarMotion kx, ScalarMotion ky)
        : state(std::make_shared<State>(std::move(kx), std::move(ky))) {
    }

    void NoiseWarpTransform::advanceFrame(TimeMillis timeInMillis) {
        state->kxSignal.advanceFrame(timeInMillis);
        state->kySignal.advanceFrame(timeInMillis);
    }

    CartesianLayer NoiseWarpTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y) {
            RawQ16_16 kx_raw = state->kxSignal.getRawValue();
            RawQ16_16 ky_raw = state->kySignal.getRawValue();

            NoiseRawU16 rawNoise = NoiseRawU16(inoise16(x, y));
            int32_t signedNoise = static_cast<int32_t>(raw(rawNoise)) - U16_HALF; // ~Q1.15 signed

            // noise (Q1.15) * k_raw (Q16.16) => Q17.31; shift 16 to yield ~Q16.16 offset in coord units
            int64_t ox = (static_cast<int64_t>(signedNoise) * raw(kx_raw)) >> 16;
            int64_t oy = (static_cast<int64_t>(signedNoise) * raw(ky_raw)) >> 16;

            uint32_t wrappedX = static_cast<uint32_t>(static_cast<int64_t>(x) + ox);
            uint32_t wrappedY = static_cast<uint32_t>(static_cast<int64_t>(y) + oy);

            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));

            return layer(finalX, finalY);
        };
    }
}
