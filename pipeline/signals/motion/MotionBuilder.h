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

#ifndef LED_SEGMENTS_PIPELINE_SIGNALS_MOTIONBUILDER_H
#define LED_SEGMENTS_PIPELINE_SIGNALS_MOTIONBUILDER_H

#include <utility>
#include "Motion.h"

namespace LEDSegments {
    inline UnboundedLinearMotion makeUnboundedLinear(Units::FracQ16_16 initialX,
                                                     Units::FracQ16_16 initialY,
                                                     Fluctuation<LinearVector> velocity) {
        return UnboundedLinearMotion(initialX, initialY, std::move(velocity));
    }

    inline UnboundedLinearMotion makeUnboundedLinear(int32_t initialX,
                                                     int32_t initialY,
                                                     Fluctuation<LinearVector> velocity) {
        return makeUnboundedLinear(Units::FracQ16_16(initialX),
                                   Units::FracQ16_16(initialY),
                                   std::move(velocity));
    }

    inline BoundedLinearMotion makeBoundedLinear(Units::FracQ16_16 initialX,
                                                 Units::FracQ16_16 initialY,
                                                 Fluctuation<LinearVector> velocity,
                                                 Units::FracQ16_16 maxRadius) {
        return BoundedLinearMotion(initialX, initialY, std::move(velocity), maxRadius);
    }

    inline BoundedLinearMotion makeBoundedLinear(int32_t initialX,
                                                 int32_t initialY,
                                                 Fluctuation<LinearVector> velocity,
                                                 Units::FracQ16_16 maxRadius) {
        return makeBoundedLinear(Units::FracQ16_16(initialX),
                                 Units::FracQ16_16(initialY),
                                 std::move(velocity),
                                 maxRadius);
    }

    inline AngularMotion makeAngular(Units::AngleUnitsQ0_16 initial,
                                     Fluctuation<Units::FracQ16_16> speed) {
        return AngularMotion(initial, std::move(speed));
    }

    inline ScalarMotion makeScalar(Fluctuation<Units::FracQ16_16> delta) {
        return ScalarMotion(std::move(delta));
    }
}

#endif //LED_SEGMENTS_PIPELINE_SIGNALS_MOTIONBUILDER_H
