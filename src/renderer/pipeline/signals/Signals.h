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

#if defined(ARDUINO) || defined(__EMSCRIPTEN__)
#include "FastLED.h"
#else
#include "native/FastLED.h"
#endif

#include "renderer/pipeline/signals/accumulators/Accumulators.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"
#include "renderer/pipeline/signals/ranges/BipolarRange.h"
#include "renderer/pipeline/signals/SignalTypes.h"

namespace PolarShader {
    using Waveform = S0x16Signal::WaveformFn;

    const MagnitudeRange<s0x16> &magnitudeRange();

    const BipolarRange<s0x16> &bipolarRange();

    S0x16Signal constant(s0x16 value);

    S0x16Signal constant(u0x16 value);

    S0x16Signal constant(uint16_t permille = 0);

    struct PeriodicSignalFactory {
        using BaseFn = S0x16Signal (*)(S0x16Signal phaseVelocity, s0x16 phaseOffset);
        using BoundedFn = S0x16Signal (*)(S0x16Signal phaseVelocity, S0x16Signal floor, S0x16Signal ceiling);
        using BoundedPhaseFn = S0x16Signal (*)(
            S0x16Signal phaseVelocity,
            s0x16 phaseOffset,
            S0x16Signal floor,
            S0x16Signal ceiling
        );

        BaseFn base{nullptr};
        BoundedFn bounded{nullptr};
        BoundedPhaseFn boundedWithPhase{nullptr};

        S0x16Signal operator()(S0x16Signal phaseVelocity, s0x16 phaseOffset) const {
            return base ? base(std::move(phaseVelocity), phaseOffset) : S0x16Signal();
        }

        S0x16Signal operator()(S0x16Signal phaseVelocity, S0x16Signal floor) const {
            return bounded ? bounded(std::move(phaseVelocity), std::move(floor), constant(1000)) : S0x16Signal();
        }

        S0x16Signal operator()(S0x16Signal phaseVelocity, S0x16Signal floor, S0x16Signal ceiling) const {
            return bounded ? bounded(std::move(phaseVelocity), std::move(floor), std::move(ceiling)) : S0x16Signal();
        }

        S0x16Signal operator()(S0x16Signal phaseVelocity, s0x16 phaseOffset, S0x16Signal floor) const {
            return boundedWithPhase
                ? boundedWithPhase(std::move(phaseVelocity), phaseOffset, std::move(floor), constant(1000))
                : S0x16Signal();
        }

        S0x16Signal operator()(
            S0x16Signal phaseVelocity,
            s0x16 phaseOffset,
            S0x16Signal floor,
            S0x16Signal ceiling
        ) const {
            return boundedWithPhase
                ? boundedWithPhase(std::move(phaseVelocity), phaseOffset, std::move(floor), std::move(ceiling))
                : S0x16Signal();
        }
    };
    using AperiodicSignalFactory = S0x16Signal (*)(TimeMillis duration, LoopMode loopMode);

    S0x16Signal cRandom();

    /** @brief Returns a wrapped signed turn offset suitable for periodic phase initialization. */
    s0x16 randomPhaseOffset();

    /**
     * @brief Animated noise signal driven by a phase velocity signal.
     * @param phaseVelocity Magnitude-domain rate where `constant(1000)` = 1.0 and `constant(500)` = 0.5.
     * @param phaseOffset Signed turn offset wrapped into the internal phase domain; zero selects a random offset.
     * @param loopPeriodMs When non-zero, the noise loops seamlessly over this period via a two-path
     *        cross-dissolve (same technique as the looping noise pattern); the random phase offset
     *        decorrelates each looping signal. Zero keeps the classic free-running noise.
     */
    S0x16Signal noise(
        S0x16Signal phaseVelocity = constant(550),
        s0x16 phaseOffset = randomPhaseOffset(),
        TimeMillis loopPeriodMs = 0
    );

    S0x16Signal noise(
        S0x16Signal phaseVelocity,
        S0x16Signal floor,
        S0x16Signal ceiling = constant(1000),
        TimeMillis loopPeriodMs = 0
    );

    S0x16Signal noise(
        S0x16Signal phaseVelocity,
        s0x16 phaseOffset,
        S0x16Signal floor,
        S0x16Signal ceiling = constant(1000),
        TimeMillis loopPeriodMs = 0
    );

    /**
     * @brief Periodic sine wave signal driven by a phase velocity signal.
     * @param phaseVelocity Magnitude-domain rate where `constant(1000)` = 1 Hz and `constant(500)` = 0.5 Hz.
     * @param phaseOffset Signed turn offset wrapped into the internal phase domain.
     * @note The overloads that accept floor/ceiling signals apply smap() internally.
     */
    S0x16Signal sine(
        S0x16Signal phaseVelocity = constant(550),
        s0x16 phaseOffset = s0x16(0)
    );

    S0x16Signal sine(
        S0x16Signal phaseVelocity,
        S0x16Signal floor,
        S0x16Signal ceiling = constant(1000)
    );

    S0x16Signal sine(
        S0x16Signal phaseVelocity,
        s0x16 phaseOffset,
        S0x16Signal floor,
        S0x16Signal ceiling = constant(1000)
    );

    S0x16Signal triangle(
        S0x16Signal phaseVelocity = constant(550),
        s0x16 phaseOffset = s0x16(0)
    );

    S0x16Signal triangle(
        S0x16Signal phaseVelocity,
        S0x16Signal floor,
        S0x16Signal ceiling = constant(1000)
    );

    S0x16Signal triangle(
        S0x16Signal phaseVelocity,
        s0x16 phaseOffset,
        S0x16Signal floor,
        S0x16Signal ceiling = constant(1000)
    );

    S0x16Signal square(
        S0x16Signal phaseVelocity = constant(550),
        s0x16 phaseOffset = s0x16(0)
    );

    S0x16Signal square(
        S0x16Signal phaseVelocity,
        S0x16Signal floor,
        S0x16Signal ceiling = constant(1000)
    );

    S0x16Signal square(
        S0x16Signal phaseVelocity,
        s0x16 phaseOffset,
        S0x16Signal floor,
        S0x16Signal ceiling = constant(1000)
    );

    S0x16Signal sawtooth(
        S0x16Signal phaseVelocity = constant(550),
        s0x16 phaseOffset = s0x16(0)
    );

    S0x16Signal sawtooth(
        S0x16Signal phaseVelocity,
        S0x16Signal floor,
        S0x16Signal ceiling = constant(1000)
    );

    S0x16Signal sawtooth(
        S0x16Signal phaseVelocity,
        s0x16 phaseOffset,
        S0x16Signal floor,
        S0x16Signal ceiling = constant(1000)
    );

    S0x16Signal linear(TimeMillis duration, LoopMode loopMode = LoopMode::RESET);

    S0x16Signal quadraticIn(TimeMillis duration, LoopMode loopMode = LoopMode::RESET);

    S0x16Signal quadraticOut(TimeMillis duration, LoopMode loopMode = LoopMode::RESET);

    S0x16Signal quadraticInOut(TimeMillis duration, LoopMode loopMode = LoopMode::RESET);

    // Scale a signed signal in the [-1..1] domain by a u0x16/s0x16 fraction.
    S0x16Signal scale(S0x16Signal signal, u0x16 factor);

    // Remap a signed signal into dynamically sampled unipolar bounds.
    // Floor and ceiling are sampled through magnitudeRange() each time the signal is sampled.
    // If floor exceeds ceiling for a given sample, the bounds are swapped before remapping.
    S0x16Signal smap(S0x16Signal signal, S0x16Signal floor, S0x16Signal ceiling);

    /** @brief Emits a constant UV coordinate. */
    UVSignal constantUV(UV value);

    /** @brief Combines two scalar signals into a 2D UV signal. */
    UVSignal uvSignal(S0x16Signal u, S0x16Signal v);

    /** @brief Maps a signed signal into a UV area via unsigned range mapping. */
    UVSignal uv(S0x16Signal signal, UV min, UV max);

    /** @brief Maps a signed signal into a UV area via unsigned range mapping. */
    UVSignal uvInRange(S0x16Signal signal, UV min, UV max);
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_SIGNALS_H
