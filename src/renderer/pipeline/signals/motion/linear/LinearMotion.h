//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

/*
 * This file is part of PolarShader.
 *
 * PolarShader is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PolarShader is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PolarShader. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_MOTION_LINEAR_H
#define POLAR_SHADER_PIPELINE_SIGNALS_MOTION_LINEAR_H

#include <utility>
#include "renderer/pipeline/signals/modulators/AngularModulators.h"
#include "renderer/pipeline/utils/Units.h"

namespace PolarShader {

    class LinearMotion {
    public:
        LinearMotion(const LinearMotion &) = default;

        LinearMotion &operator=(const LinearMotion &) = default;

        LinearMotion(LinearMotion &&) = default;

        LinearMotion &operator=(LinearMotion &&) = default;

        ~LinearMotion() = default;

        void advanceFrame(TimeMillis timeInMillis);

        int32_t getX() const { return static_cast<int32_t>(positionX.asRaw() >> 16); }
        int32_t getY() const { return static_cast<int32_t>(positionY.asRaw() >> 16); }

        RawQ16_16 getRawX() const { return RawQ16_16(positionX.asRaw()); }
        RawQ16_16 getRawY() const { return RawQ16_16(positionY.asRaw()); }

    protected:
        LinearMotion(FracQ16_16 initialX,
                     FracQ16_16 initialY,
                     ScalarModulator speed,
                     AngleModulator direction,
                     bool clampEnabled,
                     FracQ16_16 maxRadius);

        void applyRadialClamp();

        FracQ16_16 positionX;
        FracQ16_16 positionY;
        ScalarModulator speed;
        AngleModulator direction;
        TimeMillis lastTime = 0;
        bool hasLastTime = false;
        bool clampEnabled = false;
        FracQ16_16 maxRadius;
    };

    class UnboundedLinearMotion : public LinearMotion {
    public:
        UnboundedLinearMotion(FracQ16_16 initialX,
                              FracQ16_16 initialY,
                              ScalarModulator speed,
                              AngleModulator direction)
            : LinearMotion(initialX, initialY, std::move(speed), std::move(direction), false, FracQ16_16(0)) {
        }
    };

    class BoundedLinearMotion : public LinearMotion {
    public:
        BoundedLinearMotion(FracQ16_16 initialX,
                            FracQ16_16 initialY,
                            ScalarModulator speed,
                            AngleModulator direction,
                            FracQ16_16 maxRadius)
            : LinearMotion(initialX, initialY, std::move(speed), std::move(direction), true, maxRadius) {
        }
    };
}

#endif //POLAR_SHADER_PIPELINE_SIGNALS_MOTION_LINEAR_H
