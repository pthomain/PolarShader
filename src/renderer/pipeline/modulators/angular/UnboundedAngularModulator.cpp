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

#include "UnboundedAngularModulator.h"
#include "renderer/pipeline/modulators/BoundUtils.h"
#include "renderer/pipeline/utils/MathUtils.h"
#include "renderer/pipeline/utils/TimeUtils.h"

namespace PolarShader {
    UnboundedAngularModulator::UnboundedAngularModulator(
        BoundedAngle initialPhase,
        BoundedScalarSignal speed
    ) : phase(unbindAngle(initialPhase)),
        speed(std::move(speed)) {
    }

    void UnboundedAngularModulator::advanceFrame(TimeMillis timeInMillis) {
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
        RawQ16_16 phase_advance = RawQ16_16(mul_q16_16_wrap(speed_now, dt_q16).asRaw());

        wrapPolicy.apply(phase, phase_advance);
    }
}
