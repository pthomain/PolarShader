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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_MOTION_ANGULAR_MODULATOR_H
#define  POLAR_SHADER_PIPELINE_SIGNALS_MOTION_ANGULAR_MODULATOR_H

#include <renderer/pipeline/modulators/signals/ScalarSignals.h>
#include "renderer/pipeline/units/Units.h"

namespace PolarShader {
    class AngularModulator {
    public:
        inline static const SFracQ0_16 SPEED_MIN = SFracQ0_16(Q0_16_MIN);
        inline static const SFracQ0_16 SPEED_MAX = SFracQ0_16(Q0_16_MAX);

        AngularModulator(AngleQ0_16 initialPhase,
                         FracQ0_16Signal speed);

        void advanceFrame(TimeMillis timeInMillis);

        AngleQ0_16 getPhase() const { return phase; }

    private:
        // Delta-time is clamped using MAX_DELTA_TIME_MS; set to 0 to disable.
        AngleQ0_16 phase = AngleQ0_16(0);
        FracQ0_16Signal speed;
        TimeMillis lastTime = 0;
        bool hasLastTime = false;
    };
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_MOTION_ANGULAR_MODULATOR_H
