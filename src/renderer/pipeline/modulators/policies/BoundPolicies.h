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

#include <renderer/pipeline/units/Units.h>

namespace PolarShader {

    struct LinearPolicy {
        static void apply(ScalarQ0_16 &, ScalarQ0_16 &) {
        }
    };

    struct SaturatingClampPolicy {
        static void apply(ScalarQ0_16 &position_x,
                          ScalarQ0_16 &position_y,
                          int32_t dx_raw,
                          int32_t dy_raw);
    };

    struct WrapAddPolicy {
        static void apply(ScalarQ0_16 &position_x,
                          ScalarQ0_16 &position_y,
                          int32_t dx_raw,
                          int32_t dy_raw);
    };

    struct RadialClampPolicy {
        ScalarQ0_16 max_radius;

        explicit RadialClampPolicy(ScalarQ0_16 maxRadius = ScalarQ0_16(0));

        void apply(ScalarQ0_16 &position_x, ScalarQ0_16 &position_y) const;
    };

    struct ClampPolicy {
        ScalarQ0_16 min_val;
        ScalarQ0_16 max_val;

        // Default: effectively unbounded.
        ClampPolicy();

        ClampPolicy(int32_t min, int32_t max);

        ClampPolicy(ScalarQ0_16 min, ScalarQ0_16 max);

        void apply(ScalarQ0_16 &position, ScalarQ0_16 &velocity) const;
    };

    struct WrapPolicy {
        int32_t wrap_units;

        explicit WrapPolicy(int32_t wrap = Q0_16_ONE);

        void apply(ScalarQ0_16 &position, ScalarQ0_16 &) const;
    };

    struct AngleWrapPolicy {
        static void apply(AngleQ0_16 &phase, int32_t phase_advance_raw);
    };
}

#endif //POLAR_SHADER_PIPELINE_SIGNALS_SIGNALPOLICIES_H
