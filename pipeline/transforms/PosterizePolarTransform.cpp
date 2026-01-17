//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2023 Pierre Thomain

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
            AngleTurnsUQ16_16 angle_q16, RadiusQ0_16 radius) {
            AngleTurnsUQ16_16 ang = angle_q16;
            RadiusQ0_16 rad = radius;

            if (angleBins > 1) {
                uint32_t binWidth = static_cast<uint32_t>(PHASE_FULL_TURN / angleBins);
                ang = quantizePhase(angle_q16, binWidth);
            }
            if (radiusBins > 1) {
                uint32_t binWidth = static_cast<uint32_t>(FRACT_Q0_16_MAX) / (radiusBins - 1);
                uint32_t r = raw(radius);
                uint32_t quant = (r / binWidth) * binWidth;
                rad = RadiusQ0_16(static_cast<uint16_t>(quant));
            }

            return layer(ang, rad);
        };
    }
}
