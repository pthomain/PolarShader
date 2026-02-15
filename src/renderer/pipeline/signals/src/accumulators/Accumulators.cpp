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

#include "renderer/pipeline/signals/accumulators/Accumulators.h"
#include "renderer/pipeline/maths/AngleMaths.h"

namespace PolarShader {
    PhaseAccumulator::PhaseAccumulator(
        SpeedSampleFn speed,
        f16 initialPhase
    ) : phaseRaw32(static_cast<uint32_t>(raw(initialPhase)) << 16),
        phaseSpeed(std::move(speed)) {
    }

    f16 PhaseAccumulator::advance(TimeMillis elapsedMs) {
        return f16(static_cast<uint16_t>(advanceRaw(elapsedMs) >> 16));
    }

    uint32_t PhaseAccumulator::advanceRaw(TimeMillis elapsedMs) {
        if (!hasLastElapsed) {
            lastElapsedMs = elapsedMs;
            hasLastElapsed = true;
            return phaseRaw32;
        }

        int64_t deltaMs = static_cast<int64_t>(elapsedMs) - static_cast<int64_t>(lastElapsedMs);
        lastElapsedMs = elapsedMs;

        if (MAX_DELTA_TIME_MS != 0) {
            const int64_t maxDelta = MAX_DELTA_TIME_MS;
            if (deltaMs > maxDelta) deltaMs = maxDelta;
            if (deltaMs < -maxDelta) deltaMs = -maxDelta;
        }

        if (deltaMs == 0) return phaseRaw32;

        int64_t speedRaw = raw(phaseSpeed ? phaseSpeed(elapsedMs) : sf16(0));
        
        // Accumulate phase using a 32-bit container where the top 16 bits represent the 
        // integer phase (turns) and the bottom 16 bits provide fractional precision.
        // speedRaw is in sf16 (1.0 = 65536 turns/sec).
        // increment = (speed * deltaMs / 1000) * 65536
        int64_t increment = (speedRaw * deltaMs * 65536LL + 500LL) / 1000LL;
        phaseRaw32 = static_cast<uint32_t>(static_cast<int64_t>(phaseRaw32) + increment);

        return phaseRaw32;
    }
}
