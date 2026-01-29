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

#include "renderer/pipeline/signals/Accumulators.h"
#include "renderer/pipeline/maths/AngleMaths.h"
#include "renderer/pipeline/maths/Maths.h"

namespace PolarShader {
    PhaseAccumulator::PhaseAccumulator(
        MappedSignal<SFracQ0_16> speed,
        SFracQ0_16 initialPhase
    ) : phaseRaw32(static_cast<uint32_t>(raw(initialPhase)) << 16),
        phaseSpeed(std::move(speed)) {
    }

    SFracQ0_16 PhaseAccumulator::advance(TimeMillis time) {
        if (!hasLastTime) {
            lastTime = time;
            hasLastTime = true;
            return SFracQ0_16(static_cast<int32_t>(phaseRaw32 >> 16));
        }
        TimeMillis deltaTime = time - lastTime;
        lastTime = time;
        if (deltaTime == 0) return SFracQ0_16(static_cast<int32_t>(phaseRaw32 >> 16));

        deltaTime = clampDeltaTime(deltaTime);

        SFracQ0_16 dt_q0_16 = timeMillisToScalar(deltaTime);
        int64_t increment = static_cast<int64_t>(raw(phaseSpeed(time).get())) *
                            static_cast<int64_t>(raw(dt_q0_16));
        phaseRaw32 = static_cast<uint32_t>(static_cast<int64_t>(phaseRaw32) + increment);
        return SFracQ0_16(static_cast<int32_t>(phaseRaw32 >> 16));
    }

    CartesianMotionAccumulator::CartesianMotionAccumulator(
        SPoint32 initialPosition,
        MappedSignal<FracQ0_16> direction,
        MappedSignal<int32_t> speed
    ) : pos(initialPosition),
        directionSignal(std::move(direction)),
        speedSignal(std::move(speed)) {
    }

    SPoint32 CartesianMotionAccumulator::advance(TimeMillis time) {
        if (!hasLastTime) {
            lastTime = time;
            hasLastTime = true;
            return pos;
        }
        TimeMillis deltaTime = time - lastTime;
        lastTime = time;
        if (deltaTime == 0) return pos;

        deltaTime = clampDeltaTime(deltaTime);

        SFracQ0_16 dt_q0_16 = timeMillisToScalar(deltaTime);
        FracQ0_16 direction = directionSignal(time).get();
        int32_t speed = speedSignal(time).get();

        TrigQ0_16 cos_q0_16 = angleCosQ0_16(direction);
        TrigQ0_16 sin_q0_16 = angleSinQ0_16(direction);

        int64_t dx = static_cast<int64_t>(speed) * static_cast<int64_t>(raw(cos_q0_16));
        int64_t dy = static_cast<int64_t>(speed) * static_cast<int64_t>(raw(sin_q0_16));

        dx = (dx >= 0) ? (dx + (1LL << 15)) : (dx - (1LL << 15));
        dy = (dy >= 0) ? (dy + (1LL << 15)) : (dy - (1LL << 15));
        dx >>= 16;
        dy >>= 16;

        dx = dx * static_cast<int64_t>(raw(dt_q0_16));
        dy = dy * static_cast<int64_t>(raw(dt_q0_16));
        dx += (dx >= 0) ? (1LL << 15) : -(1LL << 15);
        dy += (dy >= 0) ? (1LL << 15) : -(1LL << 15);
        dx >>= 16;
        dy >>= 16;

        pos.x = static_cast<int32_t>(static_cast<int64_t>(pos.x) + dx);
        pos.y = static_cast<int32_t>(static_cast<int64_t>(pos.y) + dy);
        return pos;
    }
}
