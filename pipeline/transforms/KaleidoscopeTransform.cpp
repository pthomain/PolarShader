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

#include "KaleidoscopeTransform.h"

namespace LEDSegments {
    PolarLayer KaleidoscopeTransform::operator()(const PolarLayer &layer) const {
        return [nbSegments = this->nbSegments, isMandala = this->isMandala, isMirroring = this->isMirroring, layer](
            Units::AngleTurnsUQ16_16 angle_q16, Units::FracQ0_16 radius) {
            auto clamp_u8 = [](uint8_t v, uint8_t lo, uint8_t hi) -> uint8_t {
                if (v < lo) return lo;
                if (v > hi) return hi;
                return v;
            };
            uint8_t segments = clamp_u8(nbSegments, 1, MAX_SEGMENTS);

            Units::AngleTurnsUQ16_16 newAngle_q16;
            if (isMandala) {
                // Multiply the angle by the number of segments. The uint32_t accumulator
                // will naturally wrap, creating the intended mandala effect.
                newAngle_q16 = angle_q16 * segments;
            } else {
                newAngle_q16 = foldPhaseKaleidoscope(angle_q16, segments, isMirroring);
            }

            return layer(newAngle_q16, radius);
        };
    }
}
