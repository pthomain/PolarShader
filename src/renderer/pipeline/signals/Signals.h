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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_SIGNALS_H
#define POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_SIGNALS_H

#ifdef ARDUINO
#include "FastLED.h"
#else
#include "native/FastLED.h"
#endif

#include "renderer/pipeline/signals/accumulators/Accumulators.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"
#include "renderer/pipeline/signals/ranges/BipolarRange.h"
#include "renderer/pipeline/signals/SignalTypes.h"

namespace PolarShader {
    using Waveform = Sf16Signal::WaveformFn;

    const MagnitudeRange<sf16> &magnitudeRange();

    const BipolarRange<sf16> &bipolarRange();

    using PeriodicSignalFactory = Sf16Signal (*)(
        Sf16Signal speed,
        Sf16Signal amplitude,
        Sf16Signal offset,
        Sf16Signal phaseOffset
    );
    using AperiodicSignalFactory = Sf16Signal (*)(TimeMillis duration, LoopMode loopMode);

    Sf16Signal floor();

    Sf16Signal midPoint();

    Sf16Signal ceiling();

    Sf16Signal constant(sf16 value);

    Sf16Signal constant(f16 value);

    /**
     * @brief Creates a constant signed sf16 from per-mille input.
     *
     * Accepts signed input in [-1000, 1000]. Magnitude is clamped to 1000,
     * preserving sign.
     */
    Sf16Signal csPerMil(int16_t value);

    /**
     * @brief Creates a constant signed sf16 from unsigned per-mille input.
     *
     * Accepts [0, 1000], maps linearly to [-1000, 1000] (0 -> -1000, 500 -> 0,
     * 1000 -> 1000), then emits the equivalent constant sf16 signal.
     */
    Sf16Signal cPerMil(uint16_t value);

    Sf16Signal cRandom();

    /**
     * @brief Animated noise signal driven by a speed signal.
     * @param speed Signed speed in turns-per-second (1.0 = 1 cycle/sec).
     */
    Sf16Signal noise(
        Sf16Signal speed = csPerMil(100),
        Sf16Signal amplitude = ceiling(),
        Sf16Signal offset = midPoint(),
        Sf16Signal phaseOffset = cRandom()
    );

    /**
     * @brief Periodic sine wave signal driven by a speed signal.
     * @param speed Signed speed in turns-per-second (1.0 = 1 cycle/sec).
     */
    Sf16Signal sine(
        Sf16Signal speed = csPerMil(100),
        Sf16Signal amplitude = ceiling(),
        Sf16Signal offset = midPoint(),
        Sf16Signal phaseOffset = floor()
    );

    Sf16Signal triangle(
        Sf16Signal speed = csPerMil(100),
        Sf16Signal amplitude = ceiling(),
        Sf16Signal offset = midPoint(),
        Sf16Signal phaseOffset = floor()
    );

    Sf16Signal square(
        Sf16Signal speed = csPerMil(100),
        Sf16Signal amplitude = ceiling(),
        Sf16Signal offset = midPoint(),
        Sf16Signal phaseOffset = floor()
    );

    Sf16Signal sawtooth(
        Sf16Signal speed = csPerMil(100),
        Sf16Signal amplitude = ceiling(),
        Sf16Signal offset = midPoint(),
        Sf16Signal phaseOffset = floor()
    );

    Sf16Signal linear(TimeMillis duration, LoopMode loopMode = LoopMode::RESET);

    Sf16Signal quadraticIn(TimeMillis duration, LoopMode loopMode = LoopMode::RESET);

    Sf16Signal quadraticOut(TimeMillis duration, LoopMode loopMode = LoopMode::RESET);

    Sf16Signal quadraticInOut(TimeMillis duration, LoopMode loopMode = LoopMode::RESET);

    // Scale a signed signal in the [-1..1] domain by a f16/sf16 fraction.
    Sf16Signal scale(Sf16Signal signal, f16 factor);

    /** @brief Emits a constant UV coordinate. */
    UVSignal constantUV(UV value);

    /** @brief Combines two scalar signals into a 2D UV signal. */
    UVSignal uvSignal(Sf16Signal u, Sf16Signal v);

    /** @brief Maps a signed signal into a UV area via unsigned range mapping. */
    UVSignal uv(Sf16Signal signal, UV min, UV max);

    /** @brief Maps a signed signal into a UV area via unsigned range mapping. */
    UVSignal uvInRange(Sf16Signal signal, UV min, UV max);

    // Depth signals for animating noise domains (unsigned r8).
    DepthSignal constantDepth(uint32_t value);

    // Map a signed signal into the unsigned r8 depth domain.
    DepthSignal depth(
        Sf16Signal signal = cPerMil(100),
        MagnitudeRange<uint32_t> range = MagnitudeRange<uint32_t>(0, 1000)
    );
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_SIGNALS_H
