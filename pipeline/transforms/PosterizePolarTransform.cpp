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

#include "PosterizePolarTransform.h"

namespace LEDSegments {
    PosterizePolarTransform::PosterizePolarTransform(uint8_t angleBins, uint8_t radiusBins)
        : angleBins(angleBins), radiusBins(radiusBins) {
    }

    PolarLayer PosterizePolarTransform::operator()(const PolarLayer &layer) const {
        return [angleBins = this->angleBins, radiusBins = this->radiusBins, layer](
            Units::PhaseTurnsUQ16_16 angle_q16, Units::FractQ0_16 radius) {
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

            return layer(ang, rad);
        };
    }
}
