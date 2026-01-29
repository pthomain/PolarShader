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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_SIGNAL_TYPES_H
#define POLAR_SHADER_PIPELINE_SIGNALS_SIGNAL_TYPES_H

#include "FastLED.h"
#include "renderer/pipeline/units/ScalarUnits.h"
#include "renderer/pipeline/units/StrongTypes.h"
#include "renderer/pipeline/units/TimeUnits.h"

namespace PolarShader {
    /**
     * @brief Time-indexed scalar signal bounded to Q0.16.
     *
     * Use when:
     * - The consumer maps the output into its own min/max range.
     */
    using SFracQ0_16Signal = fl::function<SFracQ0_16(TimeMillis)>;

    /**
     * @brief Time-indexed signal that emits mapped values.
     */
    template<typename T>
    using MappedSignal = fl::function<MappedValue<T>(TimeMillis)>;
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_SIGNAL_TYPES_H
