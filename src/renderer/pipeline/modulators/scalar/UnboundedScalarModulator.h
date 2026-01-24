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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_MOTION_UNBOUNDED_SCALAR_MODULATOR_H
#define POLAR_SHADER_PIPELINE_SIGNALS_MOTION_UNBOUNDED_SCALAR_MODULATOR_H

#include "../policies/BoundPolicies.h"
#include "../signals/AngularSignals.h"
#include "../signals/ScalarSignals.h"
#include "renderer/pipeline/units/Units.h"

namespace PolarShader {

    class UnboundedScalarModulator {
    public:
        inline static const UnboundedScalar SPEED_MIN = UnboundedScalar(0);
        inline static const UnboundedScalar SPEED_MAX = UnboundedScalar(Q0_16_MAX);

        UnboundedScalarModulator(const UnboundedScalarModulator &) = default;

        UnboundedScalarModulator &operator=(const UnboundedScalarModulator &) = default;

        UnboundedScalarModulator(UnboundedScalarModulator &&) = default;

        UnboundedScalarModulator &operator=(UnboundedScalarModulator &&) = default;

        ~UnboundedScalarModulator() = default;

        void advanceFrame(TimeMillis timeInMillis);

        int32_t getX() const { return raw(positionX); }
        int32_t getY() const { return raw(positionY); }

        UnboundedScalarModulator(UnboundedScalar initialX,
                                 UnboundedScalar initialY,
                                 FracQ0_16Signal speed,
                                 AngleQ0_16Signal direction);

        UnboundedScalar positionX;
        UnboundedScalar positionY;
        FracQ0_16Signal speed;
        AngleQ0_16Signal direction;
        TimeMillis lastTime = 0;
        bool hasLastTime = false;
        WrapAddPolicy wrapPolicy;
    };
}

#endif //POLAR_SHADER_PIPELINE_SIGNALS_MOTION_UNBOUNDED_SCALAR_MODULATOR_H
