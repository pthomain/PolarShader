//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#include "BendTransform.h"
#include <cstring>
#include <cstdlib>
#include <cstdint>

namespace LEDSegments {

    struct BendTransform::State {
        LinearSignal kxSignal;
        LinearSignal kySignal;

        State(LinearSignal kx, LinearSignal ky)
            : kxSignal(std::move(kx)), kySignal(std::move(ky)) {}
    };

    BendTransform::BendTransform(LinearSignal kx, LinearSignal ky)
        : state(std::make_shared<State>(std::move(kx), std::move(ky))) {
    }

    void BendTransform::advanceFrame(Units::TimeMillis timeInMillis) {
        state->kxSignal.advanceFrame(timeInMillis);
        state->kySignal.advanceFrame(timeInMillis);
    }

    CartesianLayer BendTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y, Units::TimeMillis t) {
            Units::RawSignalQ16_16 kx_raw = state->kxSignal.getRawValue();
            Units::RawSignalQ16_16 ky_raw = state->kySignal.getRawValue();

            auto clamp_i64 = [](int64_t v, int64_t lo, int64_t hi) -> int64_t {
                if (v < lo) return lo;
                if (v > hi) return hi;
                return v;
            };

            int64_t bendX = static_cast<int64_t>(y) * static_cast<int64_t>(y);
            int64_t bendY = static_cast<int64_t>(x) * static_cast<int64_t>(x);

            // Prevent UB by clamping before multiplying by potentially large k*_raw.
            if (kx_raw != 0) {
                int64_t limit = INT64_MAX / (std::llabs(static_cast<int64_t>(kx_raw)) + 1);
                bendX = clamp_i64(bendX, -limit, limit);
                bendX = (bendX * kx_raw) >> 16;
            } else {
                bendX = 0;
            }

            if (ky_raw != 0) {
                int64_t limit = INT64_MAX / (std::llabs(static_cast<int64_t>(ky_raw)) + 1);
                bendY = clamp_i64(bendY, -limit, limit);
                bendY = (bendY * ky_raw) >> 16;
            } else {
                bendY = 0;
            }

            uint32_t wrappedX = static_cast<uint32_t>(static_cast<int64_t>(x) + bendX);
            uint32_t wrappedY = static_cast<uint32_t>(static_cast<int64_t>(y) + bendY);

            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));

            return layer(finalX, finalY, t);
        };
    }
}
