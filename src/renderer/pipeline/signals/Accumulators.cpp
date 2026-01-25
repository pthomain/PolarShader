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
#include "renderer/pipeline/maths/Maths.h"

namespace PolarShader {
    PhaseAccumulator::PhaseAccumulator(
        SFracQ0_16Signal velocity,
        SFracQ0_16 initialPhase
    ) : phaseRaw32(static_cast<uint32_t>(raw(initialPhase)) << 16),
        phaseVelocity(std::move(velocity)) {
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
        int64_t increment = static_cast<int64_t>(raw(phaseVelocity(time))) * static_cast<int64_t>(raw(dt_q0_16));
        phaseRaw32 = static_cast<uint32_t>(static_cast<int64_t>(phaseRaw32) + increment);
        return SFracQ0_16(static_cast<int32_t>(phaseRaw32 >> 16));
    }

    CartesianMotionAccumulator::CartesianMotionAccumulator(
        SPoint32 initialPosition,
        Range velocityRange,
        SFracQ0_16Signal direction,
        SFracQ0_16Signal velocity
    ) : pos(initialPosition),
        velocityRange(std::move(velocityRange)),
        directionSignal(std::move(direction)),
        velocitySignal(std::move(velocity)) {
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
        if (deltaTime == 0) return pos;

        SFracQ0_16 dt_q0_16 = timeMillisToScalar(deltaTime);
        SFracQ0_16 direction = directionSignal(time);
        SFracQ0_16 velocity = velocitySignal(time);
        SPoint32 velocity_vec = velocityRange.mapCartesian(direction, velocity);

        int64_t dx = static_cast<int64_t>(velocity_vec.x) * static_cast<int64_t>(raw(dt_q0_16));
        int64_t dy = static_cast<int64_t>(velocity_vec.y) * static_cast<int64_t>(raw(dt_q0_16));
        dx += (dx >= 0) ? (1LL << 15) : -(1LL << 15);
        dy += (dy >= 0) ? (1LL << 15) : -(1LL << 15);
        dx >>= 16;
        dy >>= 16;

        pos.x = static_cast<int32_t>(static_cast<int64_t>(pos.x) + dx);
        pos.y = static_cast<int32_t>(static_cast<int64_t>(pos.y) + dy);
        return pos;
    }
}
