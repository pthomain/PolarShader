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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_SIGNALPOLICIES_H
#define POLAR_SHADER_PIPELINE_SIGNALS_SIGNALPOLICIES_H

#include <renderer/pipeline/utils/Units.h>

namespace PolarShader {
    struct LinearPolicy {
        void apply(FracQ16_16 &, FracQ16_16 &) const {
        }
    };

    struct ClampPolicy {
        FracQ16_16 min_val;
        FracQ16_16 max_val;

        // Default: effectively unbounded.
        ClampPolicy();

        ClampPolicy(int32_t min, int32_t max);

        ClampPolicy(FracQ16_16 min, FracQ16_16 max);

        void apply(FracQ16_16 &position, FracQ16_16 &velocity) const;
    };

    struct WrapPolicy {
        int32_t wrap_units;

        explicit WrapPolicy(int32_t wrap = 65536);

        void apply(FracQ16_16 &position, FracQ16_16 &) const;
    };
}

#endif //POLAR_SHADER_PIPELINE_SIGNALS_SIGNALPOLICIES_H
