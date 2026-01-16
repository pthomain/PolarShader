//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#include "TimeQuantizeTransform.h"

namespace LEDSegments {

    TimeQuantizeTransform::TimeQuantizeTransform(uint16_t stepMillis)
        : stepMillis(stepMillis == 0 ? 1 : stepMillis) {}

    PolarLayer TimeQuantizeTransform::operator()(const PolarLayer &layer) const {
        return [step = this->stepMillis, layer](Units::PhaseTurnsUQ16_16 angle_q16, Units::FractQ0_16 radius, Units::TimeMillis timeInMillis) {
            Units::TimeMillis qt = (timeInMillis / step) * step;
            return layer(angle_q16, radius, qt);
        };
    }
}
