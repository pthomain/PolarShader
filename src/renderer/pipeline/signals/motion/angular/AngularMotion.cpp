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

#include "AngularMotion.h"
#include "renderer/pipeline/utils/MathUtils.h"
#include "renderer/pipeline/utils/TimeUtils.h"

namespace PolarShader {
    AngularMotion::AngularMotion(AngleUnitsQ0_16 initial,
                                 ScalarModulator speed)
        : phase(angleUnitsToAngleTurns(initial)),
          speed(std::move(speed)) {
    }

    void AngularMotion::advanceFrame(TimeMillis timeInMillis) {
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

        FracQ16_16 dt_q16 = millisToQ16_16(deltaTime);
        RawQ16_16 phase_advance = RawQ16_16(mul_q16_16_wrap(speed(timeInMillis), dt_q16).asRaw());
        phase = wrapAddSigned(phase, raw(phase_advance));
    }
}
