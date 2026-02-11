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
        FracQ0_16 initialPhase
    ) : phaseRaw32(static_cast<uint32_t>(raw(initialPhase)) << 16),
        phaseSpeed(std::move(speed)) {
    }

    FracQ0_16 PhaseAccumulator::advance(TimeMillis elapsedMs) {
        if (!hasLastElapsed) {
            lastElapsedMs = elapsedMs;
            hasLastElapsed = true;
            return FracQ0_16(static_cast<uint16_t>(phaseRaw32 >> 16));
        }

        int64_t deltaMs = static_cast<int64_t>(elapsedMs) - static_cast<int64_t>(lastElapsedMs);
        lastElapsedMs = elapsedMs;
        
        // We allow negative deltaMs if elapsedMs resets (e.g. scene loop), 
        // but typically it should be positive or zero.
        if (deltaMs == 0) return FracQ0_16(static_cast<uint16_t>(phaseRaw32 >> 16));

        int64_t speedRaw = raw(phaseSpeed ? phaseSpeed(elapsedMs) : SFracQ0_16(0));
        
        // Accumulate phase using a 32-bit container where the top 16 bits represent the 
        // integer phase (turns) and the bottom 16 bits provide fractional precision.
        // speedRaw is in SFracQ0_16 (1.0 = 65536 turns/sec).
        // increment = (speed * deltaMs / 1000) * 65536
        int64_t increment = (speedRaw * deltaMs * 65536LL) / 1000LL;
        phaseRaw32 = static_cast<uint32_t>(static_cast<int64_t>(phaseRaw32) + increment);

        return FracQ0_16(static_cast<uint16_t>(phaseRaw32 >> 16));
    }
}
