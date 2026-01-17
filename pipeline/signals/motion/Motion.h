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
#include "polar/pipeline/signals/Modulation.h"
#include "polar/pipeline/utils/Units.h"

namespace LEDSegments {
    class LinearMotion {
    public:
        LinearMotion(const LinearMotion &) = default;

        LinearMotion &operator=(const LinearMotion &) = default;

        LinearMotion(LinearMotion &&) = default;

        LinearMotion &operator=(LinearMotion &&) = default;

        ~LinearMotion() = default;

        void advanceFrame(TimeMillis timeInMillis);

        int32_t getX() const { return positionX.asInt(); }
        int32_t getY() const { return positionY.asInt(); }

        RawQ16_16 getRawX() const { return RawQ16_16(positionX.asRaw()); }
        RawQ16_16 getRawY() const { return RawQ16_16(positionY.asRaw()); }

    protected:
        LinearMotion(FracQ16_16 initialX,
                     FracQ16_16 initialY,
                     Modulation<LinearVector> velocity,
                     bool clampEnabled,
                     FracQ16_16 maxRadius);

        void applyRadialClamp();

        FracQ16_16 positionX;
        FracQ16_16 positionY;
        Modulation<LinearVector> velocity;
        TimeMillis lastTime = 0;
        bool hasLastTime = false;
        bool clampEnabled = false;
        FracQ16_16 maxRadius;
    };

    class UnboundedLinearMotion : public LinearMotion {
    public:
        UnboundedLinearMotion(FracQ16_16 initialX,
                              FracQ16_16 initialY,
                              Modulation<LinearVector> velocity)
            : LinearMotion(initialX, initialY, std::move(velocity), false, FracQ16_16(0)) {
        }
    };

    class BoundedLinearMotion : public LinearMotion {
    public:
        BoundedLinearMotion(FracQ16_16 initialX,
                            FracQ16_16 initialY,
                            Modulation<LinearVector> velocity,
                            FracQ16_16 maxRadius)
            : LinearMotion(initialX, initialY, std::move(velocity), true, maxRadius) {
        }
    };

    class AngularMotion {
    public:
        AngularMotion(AngleUnitsQ0_16 initial,
                      Modulation<FracQ16_16> speed);

        void advanceFrame(TimeMillis timeInMillis);

        AngleTurnsUQ16_16 getPhase() const { return phase; }

        AngleUnitsQ0_16 getAngle() const { return angleTurnsToAngleUnits(phase); }

    private:
        // Delta-time is clamped using MAX_DELTA_TIME_MS; set to 0 to disable.
        AngleTurnsUQ16_16 phase = AngleTurnsUQ16_16(0);
        Modulation<FracQ16_16> speed;
        TimeMillis lastTime = 0;
        bool hasLastTime = false;
    };

    class ScalarMotion {
    public:
        explicit ScalarMotion(Modulation<FracQ16_16> delta);

        void advanceFrame(TimeMillis timeInMillis);

        int32_t getValue() const { return value.asInt(); }

        RawQ16_16 getRawValue() const { return RawQ16_16(value.asRaw()); }

    private:
        FracQ16_16 value = FracQ16_16(0);
        Modulation<FracQ16_16> delta;
    };
}

#endif //LED_SEGMENTS_PIPELINE_SIGNALS_MOTION_H
