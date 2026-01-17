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

#ifndef LED_SEGMENTS_PIPELINE_SIGNALS_LINEARVECTOR_H
#define LED_SEGMENTS_PIPELINE_SIGNALS_LINEARVECTOR_H

#include "polar/pipeline/utils/Units.h"

namespace LEDSegments {
    class LinearVector {
    public:
        static LinearVector fromVelocity(Units::FracQ16_16 speed,
                                         Units::AngleUnitsQ0_16 direction);

        static LinearVector fromXY(Units::FracQ16_16 x, Units::FracQ16_16 y);

        Units::FracQ16_16 getX() const { return x; }
        Units::FracQ16_16 getY() const { return y; }
        Units::FracQ16_16 getSpeed() const { return speed; }
        Units::AngleUnitsQ0_16 getDirection() const { return direction; }

    private:
        LinearVector(Units::FracQ16_16 x,
                     Units::FracQ16_16 y,
                     Units::FracQ16_16 speed,
                     Units::AngleUnitsQ0_16 direction);

        Units::FracQ16_16 x;
        Units::FracQ16_16 y;
        Units::FracQ16_16 speed;
        Units::AngleUnitsQ0_16 direction;
    };
}

#endif // LED_SEGMENTS_PIPELINE_SIGNALS_LINEARVECTOR_H
