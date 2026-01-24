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

#include "FracModulator.h"
#include "renderer/pipeline/maths/Maths.h"

namespace PolarShader {
    FracModulator::FracModulator(
        FracQ0_16 initialX,
        FracQ0_16 initialY,
        FracQ0_16Signal speed,
        AngleQ0_16Signal direction
    ) : positionX(initialX),
        positionY(initialY),
        speed(std::move(speed)),
        direction(std::move(direction)) {
    }

    void FracModulator::advanceFrame(TimeMillis timeInMillis) {
        if (!hasLastTime) {
            lastTime = timeInMillis;
            hasLastTime = true;
            return;
        }
        TimeMillis deltaTime = timeInMillis - lastTime;
        lastTime = timeInMillis;
        if (deltaTime == 0) return;

        deltaTime = clampDeltaTime(deltaTime);

        SFracQ0_16 dt_q0_16 = timeMillisToScalar(deltaTime);
        SFracQ0_16 speed_now = toScalar(speed(timeInMillis), SPEED_MIN, SPEED_MAX);

        SFracQ0_16 distance = scalarMulQ0_16Sat(speed_now, dt_q0_16);
        AngleQ0_16 phase = direction(timeInMillis);
        TrigQ0_16 cos_val = angleCosQ0_16(phase);
        TrigQ0_16 sin_val = angleSinQ0_16(phase);
        int32_t dx_raw = scalarScaleQ0_16ByTrig(distance, cos_val);
        int32_t dy_raw = scalarScaleQ0_16ByTrig(distance, sin_val);

        int32_t new_x = static_cast<int32_t>(raw(positionX)) + dx_raw;
        int32_t new_y = static_cast<int32_t>(raw(positionY)) + dy_raw;
        positionX = FracQ0_16(static_cast<uint16_t>(new_x));
        positionY = FracQ0_16(static_cast<uint16_t>(new_y));
    }
}
