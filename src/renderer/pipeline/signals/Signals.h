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

#include "renderer/pipeline/signals/Accumulators.h"
#include "renderer/pipeline/ranges/LinearRange.h"
#include "renderer/pipeline/signals/SignalTypes.h"

namespace PolarShader {
    using SampleSignal = fl::function<SFracQ0_16(FracQ0_16)>;
    using PeriodicSignalFactory = SFracQ0_16Signal (*)(
        SFracQ0_16Signal speed,
        SFracQ0_16Signal amplitude,
        SFracQ0_16Signal offset,
        SFracQ0_16Signal phaseOffset
    );
    using AperiodicSignalFactory = SFracQ0_16Signal (*)(TimeMillis duration, LoopMode loopMode);

    SFracQ0_16Signal floor();

    SFracQ0_16Signal midPoint();

    SFracQ0_16Signal ceiling();

    SFracQ0_16Signal constant(SFracQ0_16 value);

    SFracQ0_16Signal constant(FracQ0_16 value);

    SFracQ0_16Signal cFrac(int32_t value);

    SFracQ0_16Signal cPerMil(int32_t value);

    SFracQ0_16Signal cRandom();

    /**
     * @brief Animated noise signal driven by a speed signal.
     * @param speed Signed speed in turns-per-second (1.0 = 1 cycle/sec).
     */
    SFracQ0_16Signal noise(
        SFracQ0_16Signal speed = cPerMil(100),
        SFracQ0_16Signal amplitude = ceiling(),
        SFracQ0_16Signal offset = floor(),
        SFracQ0_16Signal phaseOffset = floor()
    );

    /**
     * @brief Periodic sine wave signal driven by a speed signal.
     * @param speed Signed speed in turns-per-second (1.0 = 1 cycle/sec).
     */
    SFracQ0_16Signal sine(
        SFracQ0_16Signal speed = cPerMil(100),
        SFracQ0_16Signal amplitude = ceiling(),
        SFracQ0_16Signal offset = floor(),
        SFracQ0_16Signal phaseOffset = floor()
    );

    SFracQ0_16Signal linear(TimeMillis duration, LoopMode loopMode = LoopMode::RESET);

    SFracQ0_16Signal quadraticIn(TimeMillis duration, LoopMode loopMode = LoopMode::RESET);

    SFracQ0_16Signal quadraticOut(TimeMillis duration, LoopMode loopMode = LoopMode::RESET);

    SFracQ0_16Signal quadraticInOut(TimeMillis duration, LoopMode loopMode = LoopMode::RESET);

    // Scale a signal in the 0..1 domain by a Q0.16 fraction.
    SFracQ0_16Signal scale(SFracQ0_16Signal signal, FracQ0_16 factor);

    /** @brief Emits a constant UV coordinate. */
    UVSignal constantUV(UV value);

    /** @brief Combines two scalar signals into a 2D UV signal. */
    UVSignal uvSignal(SFracQ0_16Signal u, SFracQ0_16Signal v);

    /** @brief Maps a 0..1 signal into a UV area. */
    UVSignal uv(SFracQ0_16Signal signal, UV min, UV max);

    // Depth signals for animating noise domains (unsigned Q24.8).
    DepthSignal constantDepth(uint32_t value);

    // Map a 0..1 signal into the unsigned Q24.8 depth domain.
    DepthSignal depth(
        SFracQ0_16Signal signal = cPerMil(100),
        LinearRange<uint32_t> range = LinearRange<uint32_t>(0, 1000)
    );
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_SIGNALS_H
