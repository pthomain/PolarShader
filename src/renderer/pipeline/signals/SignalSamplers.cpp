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

#include "renderer/pipeline/signals/SignalSamplers.h"
#include "renderer/pipeline/maths/AngleMaths.h"
#include "renderer/pipeline/maths/NoiseMaths.h"

namespace PolarShader {
    SampleSignal sampleNoise() {
        return [](SFracQ0_16 phase) -> SFracQ0_16 {
            uint16_t phase_u16 = static_cast<uint16_t>(raw(phase));
            NoiseRawU16 rawNoise = NoiseRawU16(inoise16(angleToFastLedPhase(FracQ0_16(phase_u16))));
            PatternNormU16 normNoise = noiseNormaliseU16(rawNoise);
            int16_t signedNoise = static_cast<int16_t>(static_cast<int32_t>(raw(normNoise)) - U16_HALF);
            return SFracQ0_16(static_cast<int32_t>(signedNoise) << 1);
        };
    }

    SampleSignal sampleSine() {
        return [](SFracQ0_16 phase) -> SFracQ0_16 {
            uint16_t phase_u16 = static_cast<uint16_t>(raw(phase));
            return angleSinQ0_16(FracQ0_16(phase_u16));
        };
    }

    SampleSignal samplePulse() {
        return [](SFracQ0_16 phase) -> SFracQ0_16 {
            uint16_t saw_raw = static_cast<uint16_t>(raw(phase));
            uint16_t pulse_raw = (saw_raw < HALF_TURN_U16)
                                     ? static_cast<uint16_t>(saw_raw << 1)
                                     : static_cast<uint16_t>((ANGLE_U16_MAX - saw_raw) << 1);
            int16_t signedPulse = static_cast<int16_t>(static_cast<int32_t>(pulse_raw) - U16_HALF);
            return SFracQ0_16(static_cast<int32_t>(signedPulse) << 1);
        };
    }
}
