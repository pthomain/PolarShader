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
        return [](FracQ0_16 phase) -> SFracQ0_16 {
            NoiseRawU16 rawNoise = NoiseRawU16(inoise16(angleToFastLedPhase(phase)));
            PatternNormU16 normNoise = noiseNormaliseU16(rawNoise);
            // Span [0, 65535] mapped to SFracQ0_16. 
            return SFracQ0_16(static_cast<int32_t>(raw(normNoise)));
        };
    }

    SampleSignal sampleSine() {
        return [](FracQ0_16 phase) -> SFracQ0_16 {
            // sin16 expects 0-65535 for a full circle.
            // Result is signed 16-bit [-32768, 32767].
            int32_t s = static_cast<int32_t>(sin16(raw(phase)));
            // Map to [0, 65535]: (s + 32768) * 65535 / 65536 is roughly (s + 32768) * 2 - offset.
            // Actually, (s + 32768) already gives [0, 65535].
            return SFracQ0_16(s + 32768);
        };
    }
}
