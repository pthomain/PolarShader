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

#ifndef LED_SEGMENTS_PIPELINE_SIGNALS_MOTION_H
#define LED_SEGMENTS_PIPELINE_SIGNALS_MOTION_H

#include <utility>
#include "LinearVector.h"
#include "polar/pipeline/signals/Fluctuation.h"
#include "polar/pipeline/utils/Units.h"

namespace LEDSegments {
    class LinearMotion {
    public:
        LinearMotion(const LinearMotion &) = default;

        LinearMotion &operator=(const LinearMotion &) = default;

        LinearMotion(LinearMotion &&) = default;

        LinearMotion &operator=(LinearMotion &&) = default;

        ~LinearMotion() = default;

        void advanceFrame(Units::TimeMillis timeInMillis);

        int32_t getX() const { return positionX.asInt(); }
        int32_t getY() const { return positionY.asInt(); }

        Units::RawFracQ16_16 getRawX() const { return positionX.asRaw(); }
        Units::RawFracQ16_16 getRawY() const { return positionY.asRaw(); }

    protected:
        LinearMotion(Units::FracQ16_16 initialX,
                     Units::FracQ16_16 initialY,
                     Fluctuation<LinearVector> velocity,
                     bool clampEnabled,
                     Units::FracQ16_16 maxRadius);

        void applyRadialClamp();

        Units::FracQ16_16 positionX;
        Units::FracQ16_16 positionY;
        Fluctuation<LinearVector> velocity;
        Units::TimeMillis lastTime = 0;
        bool clampEnabled = false;
        Units::FracQ16_16 maxRadius;
    };

    class UnboundedLinearMotion : public LinearMotion {
    public:
        UnboundedLinearMotion(Units::FracQ16_16 initialX,
                              Units::FracQ16_16 initialY,
                              Fluctuation<LinearVector> velocity)
            : LinearMotion(initialX, initialY, std::move(velocity), false, Units::FracQ16_16(0)) {
        }
    };

    class BoundedLinearMotion : public LinearMotion {
    public:
        BoundedLinearMotion(Units::FracQ16_16 initialX,
                            Units::FracQ16_16 initialY,
                            Fluctuation<LinearVector> velocity,
                            Units::FracQ16_16 maxRadius)
            : LinearMotion(initialX, initialY, std::move(velocity), true, maxRadius) {
        }
    };

    class AngularMotion {
    public:
        AngularMotion(Units::AngleUnitsQ0_16 initial,
                      Fluctuation<Units::FracQ16_16> speed);

        void advanceFrame(Units::TimeMillis timeInMillis);

        Units::AngleTurnsUQ16_16 getPhase() const { return phase; }

        Units::AngleUnitsQ0_16 getAngle() const { return Units::angleTurnsToAngleUnits(phase); }

    private:
        // Delta-time is clamped using Fluctuations::MAX_DELTA_TIME_MS; set to 0 to disable.
        Units::AngleTurnsUQ16_16 phase;
        Fluctuation<Units::FracQ16_16> speed;
        Units::TimeMillis lastTime = 0;
    };

    class ScalarMotion {
    public:
        explicit ScalarMotion(Fluctuation<Units::FracQ16_16> delta);

        void advanceFrame(Units::TimeMillis timeInMillis);

        int32_t getValue() const { return value.asInt(); }

        Units::RawFracQ16_16 getRawValue() const { return value.asRaw(); }

    private:
        Units::FracQ16_16 value = Units::FracQ16_16(0);
        Fluctuation<Units::FracQ16_16> delta;
    };
}

#endif //LED_SEGMENTS_PIPELINE_SIGNALS_MOTION_H
