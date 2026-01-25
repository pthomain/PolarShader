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
#include "renderer/pipeline/units/Units.h"

namespace PolarShader {
    using SampleSignal = fl::function<TrigQ0_16(SFracQ0_16)>;

    SFracQ0_16Signal createSignal(
        SFracQ0_16Signal phaseVelocity,
        SFracQ0_16Signal amplitude,
        SFracQ0_16Signal offset,
        SampleSignal sample
    );

    /**
     * @brief Constant bounded scalar signal (Q0.16).
     */
    SFracQ0_16Signal constant(SFracQ0_16 value);

    SFracQ0_16Signal constant(FracQ0_16 value);

    SFracQ0_16Signal noise(
        SFracQ0_16Signal phaseVelocity,
        SFracQ0_16Signal amplitude,
        SFracQ0_16Signal offset = constant(SFracQ0_16(0))
    );

    SFracQ0_16Signal sine(
        SFracQ0_16Signal phaseVelocity,
        SFracQ0_16Signal amplitude,
        SFracQ0_16Signal offset = constant(SFracQ0_16(0))
    );

    SFracQ0_16Signal pulse(
        SFracQ0_16Signal phaseVelocity,
        SFracQ0_16Signal amplitude,
        SFracQ0_16Signal offset = constant(SFracQ0_16(0))
    );
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_MODULATORS_SIGNALS_H
