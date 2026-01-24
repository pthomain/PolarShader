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

#include "BoundedScalarModulator.h"
#include "renderer/pipeline/maths/Maths.h"

namespace PolarShader {
    BoundedScalarModulator::BoundedScalarModulator(
        BoundedScalar initialX,
        BoundedScalar initialY,
        BoundedScalarSignal speed,
        BoundedAngleSignal direction
    ) : positionX(initialX),
        positionY(initialY),
        speed(std::move(speed)),
        direction(std::move(direction)) {
    }

    void BoundedScalarModulator::advanceFrame(TimeMillis timeInMillis) {
        if (!hasLastTime) {
            lastTime = timeInMillis;
            hasLastTime = true;
            return;
        }
        TimeMillis deltaTime = timeInMillis - lastTime;
        lastTime = timeInMillis;
        if (deltaTime == 0) return;

        deltaTime = clampDeltaTime(deltaTime);
        if (deltaTime == 0) return;

        UnboundedScalar dt_q16 = timeMillisToScalar(deltaTime);
        UnboundedScalar speed_now = unbound(speed(timeInMillis), SPEED_MIN, SPEED_MAX);

        UnboundedScalar distance = scalarMulQ16_16Sat(speed_now, dt_q16);
        BoundedAngle phase = direction(timeInMillis);
        TrigQ1_15 cos_val = angleCosQ1_15(phase);
        TrigQ1_15 sin_val = angleSinQ1_15(phase);
        int64_t dx_raw = scalarScaleQ16_16ByTrig(RawQ16_16(distance.asRaw()), cos_val);
        int64_t dy_raw = scalarScaleQ16_16ByTrig(RawQ16_16(distance.asRaw()), sin_val);

        int32_t dx_q16 = static_cast<int32_t>(dx_raw);
        int32_t dy_q16 = static_cast<int32_t>(dy_raw);
        uint32_t new_x = static_cast<uint32_t>(raw(positionX)) + static_cast<uint32_t>(dx_q16);
        uint32_t new_y = static_cast<uint32_t>(raw(positionY)) + static_cast<uint32_t>(dy_q16);
        positionX = BoundedScalar(static_cast<uint16_t>(new_x));
        positionY = BoundedScalar(static_cast<uint16_t>(new_y));
    }
}
