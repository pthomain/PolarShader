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

#ifndef POLAR_SHADER_PIPELINE_RANGES_UVRANGE_H
#define POLAR_SHADER_PIPELINE_RANGES_UVRANGE_H

#include "renderer/pipeline/ranges/Range.h"
#include "renderer/pipeline/maths/ScalarMaths.h"

namespace PolarShader {
    /**
     * @brief Maps a 0..1 scalar signal to a 2D line segment in UV space.
     */
    class UVRange : public Range<UVRange, UV> {
    public:
        UVRange(UV min, UV max) : min_uv(min), max_uv(max) {}

        MappedValue<UV> map(SFracQ0_16 t) const override {
            uint32_t t_raw = clamp_frac_raw(raw(t));
            
            int64_t u_min = raw(min_uv.u);
            int64_t u_max = raw(max_uv.u);
            int64_t u_span = u_max - u_min;
            int64_t u_scaled = (u_span * static_cast<int64_t>(t_raw) + (1LL << 15)) >> 16;
            
            int64_t v_min = raw(min_uv.v);
            int64_t v_max = raw(max_uv.v);
            int64_t v_span = v_max - v_min;
            int64_t v_scaled = (v_span * static_cast<int64_t>(t_raw) + (1LL << 15)) >> 16;
            
            return MappedValue(UV(
                FracQ16_16(static_cast<int32_t>(u_min + u_scaled)),
                FracQ16_16(static_cast<int32_t>(v_min + v_scaled))
            ));
        }

    private:
        UV min_uv;
        UV max_uv;
    };
}

#endif // POLAR_SHADER_PIPELINE_RANGES_UVRANGE_H
