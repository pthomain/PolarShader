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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_MOTION_BOUNDED_SCALAR_MODULATOR_H
#define POLAR_SHADER_PIPELINE_SIGNALS_MOTION_BOUNDED_SCALAR_MODULATOR_H

#include <renderer/pipeline/modulators/signals/AngularSignals.h>
#include <renderer/pipeline/modulators/signals/ScalarSignals.h>
#include "renderer/pipeline/units/Units.h"

namespace PolarShader {
    class FracModulator {
    public:
        inline static const SFracQ0_16 SPEED_MIN = SFracQ0_16(0);
        inline static const SFracQ0_16 SPEED_MAX = SFracQ0_16(Q0_16_MAX);

        FracModulator(const FracModulator &) = default;

        FracModulator &operator=(const FracModulator &) = default;

        FracModulator(FracModulator &&) = default;

        FracModulator &operator=(FracModulator &&) = default;

        ~FracModulator() = default;

        void advanceFrame(TimeMillis timeInMillis);

        int32_t getX() const { return raw(positionX); }
        int32_t getY() const { return raw(positionY); }

        FracModulator(FracQ0_16 initialX,
                      FracQ0_16 initialY,
                      FracQ0_16Signal speed,
                      AngleQ0_16Signal direction);

        FracQ0_16 positionX;
        FracQ0_16 positionY;
        FracQ0_16Signal speed;
        AngleQ0_16Signal direction;
        TimeMillis lastTime = 0;
        bool hasLastTime = false;
    };
}

#endif //POLAR_SHADER_PIPELINE_SIGNALS_MOTION_BOUNDED_SCALAR_MODULATOR_H
