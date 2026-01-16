//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#include "PosterizePolarTransform.h"

namespace LEDSegments {

    PosterizePolarTransform::PosterizePolarTransform(uint8_t angleBins, uint8_t radiusBins)
        : angleBins(angleBins), radiusBins(radiusBins) {}

    PolarLayer PosterizePolarTransform::operator()(const PolarLayer &layer) const {
        return [angleBins = this->angleBins, radiusBins = this->radiusBins, layer](Units::PhaseTurnsUQ16_16 angle_q16, Units::FractQ0_16 radius, Units::TimeMillis timeInMillis) {
            Units::PhaseTurnsUQ16_16 ang = angle_q16;
            Units::FractQ0_16 rad = radius;

            if (angleBins > 1) {
                uint32_t binWidth = static_cast<uint32_t>(Units::PHASE_FULL_TURN / angleBins);
                ang = static_cast<Units::PhaseTurnsUQ16_16>((static_cast<uint64_t>(angle_q16) / binWidth) * binWidth);
            }
            if (radiusBins > 1) {
                uint32_t binWidth = static_cast<uint32_t>(Units::FRACT_Q0_16_MAX) / (radiusBins - 1);
                rad = static_cast<Units::FractQ0_16>((radius / binWidth) * binWidth);
            }

            return layer(ang, rad, timeInMillis);
        };
    }
}
