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

#ifndef POLAR_SHADER_PIPELINE_UNITS_UNITS_H
#define POLAR_SHADER_PIPELINE_UNITS_UNITS_H

#include "renderer/pipeline/units/NoiseUnits.h"
#include "renderer/pipeline/units/ScalarUnits.h"
#include "renderer/pipeline/units/TimeUnits.h"
#include "renderer/pipeline/units/UnitConstants.h"

namespace PolarShader {
    struct SPoint32 {
        int32_t x;
        int32_t y;
    };

    class Range {
    public:
        static Range polarRange(FracQ0_16 min, FracQ0_16 max);

        static Range scalarRange(int32_t min, int32_t max);

        static Range cartesianRange(int32_t radius = UINT32_MAX >> 5);

        static Range timeRange(TimeMillis durationMs = 30000);

        FracQ0_16 map(SFracQ0_16 t) const;

        int32_t mapScalar(SFracQ0_16 t) const;

        SPoint32 mapCartesian(SFracQ0_16 direction, SFracQ0_16 velocity) const;

        TimeMillis mapTime(SFracQ0_16 t) const;

    private:
        enum class Kind {
            Polar,
            Scalar,
            Cartesian,
            Time
        };

        explicit Range(Kind kind);

        Range(FracQ0_16 min, FracQ0_16 max);

        Range(int32_t min, int32_t max);

        Range(int32_t radius);

        Kind kind = Kind::Polar;
        FracQ0_16 min_frac = FracQ0_16(0);
        FracQ0_16 max_frac = FracQ0_16(FRACT_Q0_16_MAX);
        int32_t min_scalar = 0;
        int32_t max_scalar = 0;
        int32_t radius = 0;
        TimeMillis duration_ms = 0;
    };
}

#endif // POLAR_SHADER_PIPELINE_UNITS_UNITS_H
