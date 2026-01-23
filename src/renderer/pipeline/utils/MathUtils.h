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

#ifndef POLAR_SHADER_PIPELINE_UTILS_MATHUTILS_H
#define POLAR_SHADER_PIPELINE_UTILS_MATHUTILS_H

#include "Units.h"

namespace PolarShader {
    fl::i32 scale_i32_by_f16(fl::i32 value, BoundedScalar scale);

    fl::i32 scale_q16_16_by_f16(fl::i32 value_q16_16, BoundedScalar scale_f16);

    UnboundedScalar mul_q16_16_sat(UnboundedScalar a, UnboundedScalar b);

    UnboundedScalar mul_q16_16_wrap(UnboundedScalar a, UnboundedScalar b);

    RawQ16_16 add_wrap_q16_16(RawQ16_16 a, RawQ16_16 b);

    uint16_t sqrt_u32(uint32_t value);

    BoundedAngle atan2_turns_approx(int16_t y, int16_t x);

    uint64_t sqrt_u64(uint64_t value);

    int64_t scale_q16_16_by_trig(RawQ16_16 magnitude, TrigQ1_15 trig_q1_15);

    UnboundedScalar clamp_q16_16_raw(int64_t raw_value);

    inline int64_t clamp_raw_i64(int64_t value) {
        if (value > INT32_MAX) return INT32_MAX;
        if (value < INT32_MIN) return INT32_MIN;
        return value;
    }
}
#endif //POLAR_SHADER_PIPELINE_UTILS_MATHUTILS_H
