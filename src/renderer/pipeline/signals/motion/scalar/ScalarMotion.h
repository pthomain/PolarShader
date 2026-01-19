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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_MOTION_SCALAR_H
#define  POLAR_SHADER_PIPELINE_SIGNALS_MOTION_SCALAR_H

#include "renderer/pipeline/signals/modulators/ScalarModulators.h"
#include "renderer/pipeline/utils/Units.h"

namespace PolarShader {
    class ScalarMotion {
    public:
        explicit ScalarMotion(ScalarModulator delta);

        void advanceFrame(TimeMillis timeInMillis);

        int32_t getValue() const { return static_cast<int32_t>(value.asRaw() >> 16); }

        RawQ16_16 getRawValue() const { return RawQ16_16(value.asRaw()); }

    private:
        FracQ16_16 value = FracQ16_16(0);
        ScalarModulator delta;
    };
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_MOTION_SCALAR_H
