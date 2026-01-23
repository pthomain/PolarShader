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
        static void apply(UnboundedScalar &, UnboundedScalar &) {
        }
    };

    struct SaturatingClampPolicy {
        static void apply(UnboundedScalar &position_x,
                          UnboundedScalar &position_y,
                          int64_t dx_raw,
                          int64_t dy_raw);
    };

    struct WrapAddPolicy {
        static void apply(UnboundedScalar &position_x,
                          UnboundedScalar &position_y,
                          int64_t dx_raw,
                          int64_t dy_raw);
    };

    struct RadialClampPolicy {
        UnboundedScalar max_radius;

        explicit RadialClampPolicy(UnboundedScalar maxRadius = UnboundedScalar(0));

        void apply(UnboundedScalar &position_x, UnboundedScalar &position_y) const;
    };

    struct ClampPolicy {
        UnboundedScalar min_val;
        UnboundedScalar max_val;

        // Default: effectively unbounded.
        ClampPolicy();

        ClampPolicy(int32_t min, int32_t max);

        ClampPolicy(UnboundedScalar min, UnboundedScalar max);

        void apply(UnboundedScalar &position, UnboundedScalar &velocity) const;
    };

    struct WrapPolicy {
        int32_t wrap_units;

        explicit WrapPolicy(int32_t wrap = 65536);

        void apply(UnboundedScalar &position, UnboundedScalar &) const;
    };

    struct AngleWrapPolicy {
        static void apply(UnboundedAngle &phase, RawQ16_16 phase_advance);
    };
}

#endif //POLAR_SHADER_PIPELINE_SIGNALS_SIGNALPOLICIES_H
