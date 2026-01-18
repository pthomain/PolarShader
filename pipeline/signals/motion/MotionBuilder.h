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
#include "linear/LinearMotion.h"
#include "angular/AngularMotion.h"
#include "scalar/ScalarMotion.h"

namespace LEDSegments {
    inline UnboundedLinearMotion linearUnbounded(FracQ16_16 initialX,
                                                 FracQ16_16 initialY,
                                                 ScalarModulator speed,
                                                 AngleModulator direction) {
        return UnboundedLinearMotion(initialX, initialY, std::move(speed), std::move(direction));
    }

    inline UnboundedLinearMotion linearUnbounded(int32_t initialX,
                                                 int32_t initialY,
                                                 ScalarModulator speed,
                                                 AngleModulator direction) {
        return linearUnbounded(FracQ16_16(initialX),
                               FracQ16_16(initialY),
                               std::move(speed),
                               std::move(direction));
    }

    inline BoundedLinearMotion linearBounded(FracQ16_16 initialX,
                                             FracQ16_16 initialY,
                                             ScalarModulator speed,
                                             AngleModulator direction,
                                             FracQ16_16 maxRadius) {
        return BoundedLinearMotion(initialX, initialY, std::move(speed), std::move(direction), maxRadius);
    }

    inline BoundedLinearMotion linearBounded(int32_t initialX,
                                             int32_t initialY,
                                             ScalarModulator speed,
                                             AngleModulator direction,
                                             FracQ16_16 maxRadius) {
        return linearBounded(FracQ16_16(initialX),
                             FracQ16_16(initialY),
                             std::move(speed),
                             std::move(direction),
                             maxRadius);
    }

    inline AngularMotion angular(AngleUnitsQ0_16 initial,
                                 ScalarModulator speed) {
        return AngularMotion(initial, std::move(speed));
    }

    inline ScalarMotion scalar(ScalarModulator delta) {
        return ScalarMotion(std::move(delta));
    }
}

#endif //LED_SEGMENTS_PIPELINE_SIGNALS_MOTIONBUILDER_H
