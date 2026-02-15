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
        return [](f16 phase) -> sf16 {
            NoiseRawU16 rawNoise = NoiseRawU16(inoise16(angleToFastLedPhase(phase)));
            return toSigned(f16(raw(rawNoise)));
        };
    }

    SampleSignal32 sampleNoise32() {
        return [](uint32_t phase) -> sf16 {
            // inoise16 for 1D uses a 32-bit coordinate (16.16)
            NoiseRawU16 rawNoise = NoiseRawU16(inoise16(phase << 5));
            return toSigned(f16(raw(rawNoise)));
        };
    }

    SampleSignal sampleSine() {
        return [](f16 phase) -> sf16 {
            // sin16 expects 0-65535 for a full circle.
            // Result is signed 16-bit [-32768, 32767].
            int32_t s = sin16(raw(phase));
            return sf16(s << 1);
        };
    }

    SampleSignal sampleTriangle() {
        return [](f16 phase) -> sf16 {
            uint32_t p = raw(phase);
            if (p < 0x8000) {
                // 0 -> -1, 0.25 -> 0, 0.5 -> 1
                return sf16(static_cast<int32_t>(p << 2) - 65536);
            } else {
                // 0.5 -> 1, 0.75 -> 0, 1.0 -> -1
                return sf16(196608 - static_cast<int32_t>(p << 2));
            }
        };
    }

    SampleSignal sampleSquare() {
        return [](f16 phase) -> sf16 {
            return (raw(phase) < 0x8000) ? sf16(SF16_MAX) : sf16(SF16_MIN);
        };
    }

    SampleSignal sampleSawtooth() {
        return [](f16 phase) -> sf16 {
            return toSigned(phase);
        };
    }
}
