//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#include "PerspectiveWarpTransform.h"
#include <cstring>

namespace LEDSegments {

    struct PerspectiveWarpTransform::State {
        LinearSignal kSignal;

        explicit State(LinearSignal k) : kSignal(std::move(k)) {}
    };

    PerspectiveWarpTransform::PerspectiveWarpTransform(LinearSignal k)
        : state(std::make_shared<State>(std::move(k))) {
    }

    void PerspectiveWarpTransform::advanceFrame(Units::TimeMillis timeInMillis) {
        state->kSignal.advanceFrame(timeInMillis);
    }

    CartesianLayer PerspectiveWarpTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y, Units::TimeMillis t) {
            Units::RawSignalQ16_16 k_raw = state->kSignal.getRawValue();
            int64_t ky = (static_cast<int64_t>(k_raw) * y) >> 16;
            int64_t denom = static_cast<int64_t>(Units::Q16_16_ONE) + ky;

            // Clamp denom to avoid singularity and overflow.
            // Ensure minimum magnitude to prevent division by zero or explosive scaling.
            // 64 in Q16.16 is ~0.001.
            if (denom > -64 && denom < 64) {
                denom = (denom >= 0) ? 64 : -64;
            }

            int64_t scale_q16 = (static_cast<int64_t>(Units::Q16_16_ONE) << 16) / denom; // Q16.16 factor

            int64_t sx = (static_cast<int64_t>(x) * scale_q16 + (1LL << 15)) >> 16;
            int64_t sy = (static_cast<int64_t>(y) * scale_q16 + (1LL << 15)) >> 16;

            uint32_t wrappedX = static_cast<uint32_t>(sx);
            uint32_t wrappedY = static_cast<uint32_t>(sy);

            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));

            return layer(finalX, finalY, t);
        };
    }
}
