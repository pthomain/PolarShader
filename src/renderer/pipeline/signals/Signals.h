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

#include "FastLED.h"
#include "renderer/pipeline/signals/Accumulators.h"
#include "renderer/pipeline/units/AngleUnits.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include "renderer/pipeline/ranges/DepthRange.h"
#include "renderer/pipeline/units/UnitConstants.h"

namespace PolarShader {
    using SampleSignal = fl::function<TrigQ0_16(SFracQ0_16)>;

    SFracQ0_16Signal createSignal(
        SFracQ0_16Signal phaseSpeed,
        SFracQ0_16Signal amplitude,
        SFracQ0_16Signal offset,
        SampleSignal sample
    );

    SFracQ0_16Signal floor();

    SFracQ0_16Signal midPoint();

    SFracQ0_16Signal ceiling();

    SFracQ0_16Signal constant(SFracQ0_16 value);

    SFracQ0_16Signal constant(FracQ0_16 value);

    SFracQ0_16Signal cFrac(int32_t value);

    SFracQ0_16Signal cPerMil(int32_t value);

    SFracQ0_16Signal full();

    SFracQ0_16Signal noise(
        SFracQ0_16Signal phaseSpeed = constant(perMil(100)),
        SFracQ0_16Signal amplitude = constant(SFracQ0_16(Q0_16_MAX)),
        SFracQ0_16Signal offset = constant(SFracQ0_16(0))
    );

    SFracQ0_16Signal sine(
        SFracQ0_16Signal phaseSpeed = midPoint(),
        SFracQ0_16Signal amplitude = constant(SFracQ0_16(Q0_16_MAX)),
        SFracQ0_16Signal offset = constant(SFracQ0_16(0))
    );

    SFracQ0_16Signal pulse(
        SFracQ0_16Signal phaseSpeed = midPoint(),
        SFracQ0_16Signal amplitude = constant(SFracQ0_16(Q0_16_MAX)),
        SFracQ0_16Signal offset = constant(SFracQ0_16(0))
    );

    // Standard easing signals that loop over durationMs and return 0..1 in Q0.16.
    SFracQ0_16Signal linear(TimeMillis durationMs);

    SFracQ0_16Signal easeIn(TimeMillis durationMs);

    SFracQ0_16Signal easeOut(TimeMillis durationMs);

    SFracQ0_16Signal easeInOut(TimeMillis durationMs);

    // Invert a signal in the 0..1 domain: y = 1 - x.
    SFracQ0_16Signal invert(SFracQ0_16Signal signal);

    // Scale a signal in the 0..1 domain by a Q0.16 fraction.
    SFracQ0_16Signal scale(SFracQ0_16Signal signal, FracQ0_16 factor);

    // Depth signals for animating noise domains (unsigned Q24.8).
    DepthSignal constantDepth(uint32_t value);

    // Map a 0..1 signal into the unsigned Q24.8 depth domain.
    DepthSignal depth(
        SFracQ0_16Signal signal = constant(perMil(100)),
        DepthRange range = DepthRange()
    );
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_SIGNALS_H
