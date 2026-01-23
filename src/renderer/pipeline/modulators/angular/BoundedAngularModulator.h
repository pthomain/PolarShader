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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_MOTION_BOUNDED_ANGULAR_MODULATOR_H
#define  POLAR_SHADER_PIPELINE_SIGNALS_MOTION_BOUNDED_ANGULAR_MODULATOR_H

#include "../signals/ScalarSignals.h"
#include "renderer/pipeline/utils/Units.h"

namespace PolarShader {
    class BoundedAngularModulator {
    public:
        inline static const UnboundedScalar SPEED_MIN = UnboundedScalar(-1);
        inline static const UnboundedScalar SPEED_MAX = UnboundedScalar(1);

        BoundedAngularModulator(BoundedAngle initialPhase,
                                BoundedScalarSignal speed);

        void advanceFrame(TimeMillis timeInMillis);

        BoundedAngle getPhase() const { return phase; }

        BoundedAngle getAngle() const { return phase; }

    private:
        // Delta-time is clamped using MAX_DELTA_TIME_MS; set to 0 to disable.
        UnboundedAngle phase_accum = UnboundedAngle(0);
        BoundedAngle phase = BoundedAngle(0);
        BoundedScalarSignal speed;
        TimeMillis lastTime = 0;
        bool hasLastTime = false;
    };
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_MOTION_BOUNDED_ANGULAR_MODULATOR_H
