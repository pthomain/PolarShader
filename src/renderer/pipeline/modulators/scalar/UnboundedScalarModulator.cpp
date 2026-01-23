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

#include "UnboundedScalarModulator.h"
#include "renderer/pipeline/modulators/BoundUtils.h"
#include "renderer/pipeline/utils/MathUtils.h"
#include "renderer/pipeline/utils/TimeUtils.h"

namespace PolarShader {
    UnboundedScalarModulator::UnboundedScalarModulator(
        UnboundedScalar initialX,
        UnboundedScalar initialY,
        BoundedScalarSignal speed,
        BoundedAngleSignal direction
    ) : positionX(initialX),
        positionY(initialY),
        speed(std::move(speed)),
        direction(std::move(direction)) {
    }

    void UnboundedScalarModulator::advanceFrame(TimeMillis timeInMillis) {
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

        UnboundedScalar dt_q16 = millisToUnboundedScalar(deltaTime);
        UnboundedScalar speed_now = unbound(speed(timeInMillis), SPEED_MIN, SPEED_MAX);

        UnboundedScalar distance = mul_q16_16_sat(speed_now, dt_q16);
        BoundedAngle phase = direction(timeInMillis);
        TrigQ1_15 cos_val = cosQ1_15(phase);
        TrigQ1_15 sin_val = sinQ1_15(phase);
        int64_t dx_raw = scale_q16_16_by_trig(RawQ16_16(distance.asRaw()), cos_val);
        int64_t dy_raw = scale_q16_16_by_trig(RawQ16_16(distance.asRaw()), sin_val);

        wrapPolicy.apply(positionX, positionY, dx_raw, dy_raw);
    }
}
