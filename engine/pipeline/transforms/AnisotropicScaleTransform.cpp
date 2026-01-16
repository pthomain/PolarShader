//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#include "AnisotropicScaleTransform.h"
#include <cstring>

namespace LEDSegments {

    struct AnisotropicScaleTransform::State {
        LinearSignal sxSignal;
        LinearSignal sySignal;

        State(LinearSignal sx, LinearSignal sy)
            : sxSignal(std::move(sx)), sySignal(std::move(sy)) {}
    };

    AnisotropicScaleTransform::AnisotropicScaleTransform(LinearSignal sx, LinearSignal sy)
        : state(std::make_shared<State>(std::move(sx), std::move(sy))) {
    }

    void AnisotropicScaleTransform::advanceFrame(Units::TimeMillis timeInMillis) {
        state->sxSignal.advanceFrame(timeInMillis);
        state->sySignal.advanceFrame(timeInMillis);
    }

    CartesianLayer AnisotropicScaleTransform::operator()(const CartesianLayer &layer) const {
        return [state = this->state, layer](int32_t x, int32_t y, Units::TimeMillis t) {
            Units::RawSignalQ16_16 sx_raw = state->sxSignal.getRawValue();
            Units::RawSignalQ16_16 sy_raw = state->sySignal.getRawValue();

            int64_t scaledX = static_cast<int64_t>(x) * sx_raw;
            int64_t scaledY = static_cast<int64_t>(y) * sy_raw;

            uint32_t wrappedX = static_cast<uint32_t>(scaledX >> 16);
            uint32_t wrappedY = static_cast<uint32_t>(scaledY >> 16);

            int32_t finalX;
            int32_t finalY;
            memcpy(&finalX, &wrappedX, sizeof(finalX));
            memcpy(&finalY, &wrappedY, sizeof(finalY));

            return layer(finalX, finalY, t);
        };
    }
}
