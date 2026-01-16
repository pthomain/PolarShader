//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#include "NoiseWarpTransform.h"
#include <cstring>

namespace LEDSegments {

    struct NoiseWarpTransform::State {
        LinearSignal kxSignal;
        LinearSignal kySignal;

        State(LinearSignal kx, LinearSignal ky)
            : kxSignal(std::move(kx)), kySignal(std::move(ky)) {}
    };

    NoiseWarpTransform::NoiseWarpTransform(LinearSignal kx, LinearSignal ky)
        : state(std::make_shared<State>(std::move(kx), std::move(ky))) {
    }

    void NoiseWarpTransform::advanceFrame(Units::TimeMillis timeInMillis) {
        state->kxSignal.advanceFrame(timeInMillis);
        state->kySignal.advanceFrame(timeInMillis);
    }

    CartesianLayer NoiseWarpTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y, Units::TimeMillis t) {
            Units::RawSignalQ16_16 kx_raw = state->kxSignal.getRawValue();
            Units::RawSignalQ16_16 ky_raw = state->kySignal.getRawValue();

            int32_t signedNoise = static_cast<int32_t>(inoise16(x, y)) - Units::U16_HALF; // ~Q1.15 signed

            // noise (Q1.15) * k_raw (Q16.16) => Q17.31; shift 16 to yield ~Q16.16 offset in coord units
            int64_t ox = (static_cast<int64_t>(signedNoise) * kx_raw) >> 16;
            int64_t oy = (static_cast<int64_t>(signedNoise) * ky_raw) >> 16;

            uint32_t wrappedX = static_cast<uint32_t>(static_cast<int64_t>(x) + ox);
            uint32_t wrappedY = static_cast<uint32_t>(static_cast<int64_t>(y) + oy);

            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));

            return layer(finalX, finalY, t);
        };
    }
}
