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
    using Waveform = Sf16Signal::WaveformFn;

    const MagnitudeRange<sf16> &magnitudeRange();

    const BipolarRange<sf16> &bipolarRange();

    Sf16Signal constant(sf16 value);

    Sf16Signal constant(f16 value);

    Sf16Signal constant(uint16_t permille = 0);

    struct PeriodicSignalFactory {
        using BaseFn = Sf16Signal (*)(Sf16Signal phaseVelocity, sf16 phaseOffset);
        using BoundedFn = Sf16Signal (*)(Sf16Signal phaseVelocity, Sf16Signal floor, Sf16Signal ceiling);
        using BoundedPhaseFn = Sf16Signal (*)(
            Sf16Signal phaseVelocity,
            sf16 phaseOffset,
            Sf16Signal floor,
            Sf16Signal ceiling
        );

        BaseFn base{nullptr};
        BoundedFn bounded{nullptr};
        BoundedPhaseFn boundedWithPhase{nullptr};

        Sf16Signal operator()(Sf16Signal phaseVelocity, sf16 phaseOffset) const {
            return base ? base(std::move(phaseVelocity), phaseOffset) : Sf16Signal();
        }

        Sf16Signal operator()(Sf16Signal phaseVelocity, Sf16Signal floor) const {
            return bounded ? bounded(std::move(phaseVelocity), std::move(floor), constant(1000)) : Sf16Signal();
        }

        Sf16Signal operator()(Sf16Signal phaseVelocity, Sf16Signal floor, Sf16Signal ceiling) const {
            return bounded ? bounded(std::move(phaseVelocity), std::move(floor), std::move(ceiling)) : Sf16Signal();
        }

        Sf16Signal operator()(Sf16Signal phaseVelocity, sf16 phaseOffset, Sf16Signal floor) const {
            return boundedWithPhase
                ? boundedWithPhase(std::move(phaseVelocity), phaseOffset, std::move(floor), constant(1000))
                : Sf16Signal();
        }

        Sf16Signal operator()(
            Sf16Signal phaseVelocity,
            sf16 phaseOffset,
            Sf16Signal floor,
            Sf16Signal ceiling
        ) const {
            return boundedWithPhase
                ? boundedWithPhase(std::move(phaseVelocity), phaseOffset, std::move(floor), std::move(ceiling))
                : Sf16Signal();
        }
    };
    using AperiodicSignalFactory = Sf16Signal (*)(TimeMillis duration, LoopMode loopMode);

    Sf16Signal cRandom();

    /** @brief Returns a wrapped signed turn offset suitable for periodic phase initialization. */
    sf16 randomPhaseOffset();

    /**
     * @brief Animated noise signal driven by a phase velocity signal.
     * @param phaseVelocity Magnitude-domain rate where `constant(1000)` = 1.0 and `constant(500)` = 0.5.
     * @param phaseOffset Signed turn offset wrapped into the internal phase domain.
     */
    Sf16Signal noise(
        Sf16Signal phaseVelocity = constant(550),
        sf16 phaseOffset = randomPhaseOffset()
    );

    Sf16Signal noise(
        Sf16Signal phaseVelocity,
        Sf16Signal floor,
        Sf16Signal ceiling = constant(1000)
    );

    Sf16Signal noise(
        Sf16Signal phaseVelocity,
        sf16 phaseOffset,
        Sf16Signal floor,
        Sf16Signal ceiling = constant(1000)
    );

    /**
     * @brief Periodic sine wave signal driven by a phase velocity signal.
     * @param phaseVelocity Magnitude-domain rate where `constant(1000)` = 1 Hz and `constant(500)` = 0.5 Hz.
     * @param phaseOffset Signed turn offset wrapped into the internal phase domain.
     * @note The overloads that accept floor/ceiling signals apply smap() internally.
     */
    Sf16Signal sine(
        Sf16Signal phaseVelocity = constant(550),
        sf16 phaseOffset = sf16(0)
    );

    Sf16Signal sine(
        Sf16Signal phaseVelocity,
        Sf16Signal floor,
        Sf16Signal ceiling = constant(1000)
    );

    Sf16Signal sine(
        Sf16Signal phaseVelocity,
        sf16 phaseOffset,
        Sf16Signal floor,
        Sf16Signal ceiling = constant(1000)
    );

    Sf16Signal triangle(
        Sf16Signal phaseVelocity = constant(550),
        sf16 phaseOffset = sf16(0)
    );

    Sf16Signal triangle(
        Sf16Signal phaseVelocity,
        Sf16Signal floor,
        Sf16Signal ceiling = constant(1000)
    );

    Sf16Signal triangle(
        Sf16Signal phaseVelocity,
        sf16 phaseOffset,
        Sf16Signal floor,
        Sf16Signal ceiling = constant(1000)
    );

    Sf16Signal square(
        Sf16Signal phaseVelocity = constant(550),
        sf16 phaseOffset = sf16(0)
    );

    Sf16Signal square(
        Sf16Signal phaseVelocity,
        Sf16Signal floor,
        Sf16Signal ceiling = constant(1000)
    );

    Sf16Signal square(
        Sf16Signal phaseVelocity,
        sf16 phaseOffset,
        Sf16Signal floor,
        Sf16Signal ceiling = constant(1000)
    );

    Sf16Signal sawtooth(
        Sf16Signal phaseVelocity = constant(550),
        sf16 phaseOffset = sf16(0)
    );

    Sf16Signal sawtooth(
        Sf16Signal phaseVelocity,
        Sf16Signal floor,
        Sf16Signal ceiling = constant(1000)
    );

    Sf16Signal sawtooth(
        Sf16Signal phaseVelocity,
        sf16 phaseOffset,
        Sf16Signal floor,
        Sf16Signal ceiling = constant(1000)
    );

    Sf16Signal linear(TimeMillis duration, LoopMode loopMode = LoopMode::RESET);

    Sf16Signal quadraticIn(TimeMillis duration, LoopMode loopMode = LoopMode::RESET);

    Sf16Signal quadraticOut(TimeMillis duration, LoopMode loopMode = LoopMode::RESET);

    Sf16Signal quadraticInOut(TimeMillis duration, LoopMode loopMode = LoopMode::RESET);

    // Scale a signed signal in the [-1..1] domain by a f16/sf16 fraction.
    Sf16Signal scale(Sf16Signal signal, f16 factor);

    // Remap a signed signal into dynamically sampled unipolar bounds.
    // Floor and ceiling are sampled through magnitudeRange() each time the signal is sampled.
    // If floor exceeds ceiling for a given sample, the bounds are swapped before remapping.
    Sf16Signal smap(Sf16Signal signal, Sf16Signal floor, Sf16Signal ceiling);

    /** @brief Emits a constant UV coordinate. */
    UVSignal constantUV(UV value);

    /** @brief Combines two scalar signals into a 2D UV signal. */
    UVSignal uvSignal(Sf16Signal u, Sf16Signal v);

    /** @brief Maps a signed signal into a UV area via unsigned range mapping. */
    UVSignal uv(Sf16Signal signal, UV min, UV max);

    /** @brief Maps a signed signal into a UV area via unsigned range mapping. */
    UVSignal uvInRange(Sf16Signal signal, UV min, UV max);
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_SIGNALS_H
