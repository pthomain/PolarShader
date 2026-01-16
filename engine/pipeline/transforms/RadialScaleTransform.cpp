//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#include "RadialScaleTransform.h"

namespace LEDSegments {

    struct RadialScaleTransform::State {
        LinearSignal kSignal;

        explicit State(LinearSignal k) : kSignal(std::move(k)) {}
    };

    RadialScaleTransform::RadialScaleTransform(LinearSignal k)
        : state(std::make_shared<State>(std::move(k))) {
    }

    void RadialScaleTransform::advanceFrame(Units::TimeMillis timeInMillis) {
        state->kSignal.advanceFrame(timeInMillis);
    }

    PolarLayer RadialScaleTransform::operator()(const PolarLayer &layer) const {
        return [state = this->state, layer](Units::PhaseTurnsUQ16_16 angle_q16, Units::FractQ0_16 radius, Units::TimeMillis timeInMillis) {
            Units::RawSignalQ16_16 k_raw = state->kSignal.getRawValue();
            int64_t delta = (static_cast<int64_t>(k_raw) * static_cast<int64_t>(radius)) >> 16;
            int64_t new_radius = static_cast<int64_t>(radius) + delta;
            if (new_radius < 0) new_radius = 0;
            if (new_radius > Units::FRACT_Q0_16_MAX) new_radius = Units::FRACT_Q0_16_MAX;
            Units::FractQ0_16 r_out = static_cast<Units::FractQ0_16>(new_radius);
            return layer(angle_q16, r_out, timeInMillis);
        };
    }
}
