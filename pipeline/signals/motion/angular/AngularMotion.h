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

#ifndef LED_SEGMENTS_PIPELINE_SIGNALS_MOTION_ANGULAR_H
#define  LED_SEGMENTS_PIPELINE_SIGNALS_MOTION_ANGULAR_H

#include "polar/pipeline/signals/modulators/ScalarModulators.h"
#include "polar/pipeline/utils/Units.h"

namespace LEDSegments {
    class AngularMotion {
    public:
        AngularMotion(AngleUnitsQ0_16 initial,
                      ScalarModulator speed);

        void advanceFrame(TimeMillis timeInMillis);

        AngleTurnsUQ16_16 getPhase() const { return phase; }

        AngleUnitsQ0_16 getAngle() const { return angleTurnsToAngleUnits(phase); }

    private:
        // Delta-time is clamped using MAX_DELTA_TIME_MS; set to 0 to disable.
        AngleTurnsUQ16_16 phase = AngleTurnsUQ16_16(0);
        ScalarModulator speed;
        TimeMillis lastTime = 0;
        bool hasLastTime = false;
    };
}

#endif // LED_SEGMENTS_PIPELINE_SIGNALS_MOTION_ANGULAR_H
