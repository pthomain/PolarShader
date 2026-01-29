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

#include "ScalarRange.h"
#include "renderer/pipeline/units/UnitConstants.h"

namespace PolarShader {
    namespace {
        // Clamps a signed Q0.16 value to the unsigned [0, 1] range.
        uint16_t clamp_to_unsigned_q0_16(SFracQ0_16 t) {
            int32_t raw_val = raw(t);
            if (raw_val <= 0) return 0;
            if (raw_val >= FRACT_Q0_16_MAX) return FRACT_Q0_16_MAX;
            return static_cast<uint16_t>(raw_val);
        }
    }

    ScalarRange::ScalarRange(int32_t min, int32_t max)
        : min_scalar(min), max_scalar(max) {
    }

    MappedValue<int32_t> ScalarRange::map(SFracQ0_16 t) const {
        int64_t span = static_cast<int64_t>(max_scalar) - static_cast<int64_t>(min_scalar);
        if (span == 0) return MappedValue(min_scalar);

        // Map t from [0, 1] (i.e., [0, FRACT_Q0_16_MAX]) to the target scalar range.
        // This assumes a unipolar signal. Bipolar signals must be remapped to unipolar first.
        uint16_t t_raw = clamp_to_unsigned_q0_16(t);

        int64_t scaled = (span * static_cast<int64_t>(t_raw)) >> 16;
        return MappedValue(static_cast<int32_t>(static_cast<int64_t>(min_scalar) + scaled));
    }
}
