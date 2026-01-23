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

#include "renderer/pipeline/utils/MathUtils.h"

namespace PolarShader {
    UnboundedScalar unbound(BoundedScalar t, UnboundedScalar min_val, UnboundedScalar max_val) {
        int64_t min_raw = min_val.asRaw();
        int64_t max_raw = max_val.asRaw();
        int64_t span = max_raw - min_raw;
        if (span <= 0) return min_val;

        uint32_t t_raw = raw(t);
        int64_t scaled = (span * static_cast<int64_t>(t_raw)) >> 16;
        return clamp_q16_16_raw(min_raw + scaled);
    }

    BoundedScalar bound(UnboundedScalar value, UnboundedScalar min_val, UnboundedScalar max_val) {
        int64_t min_raw = min_val.asRaw();
        int64_t max_raw = max_val.asRaw();
        int64_t span = max_raw - min_raw;
        if (span <= 0) return BoundedScalar(0);

        int64_t val_raw = value.asRaw();
        if (val_raw < min_raw) val_raw = min_raw;
        if (val_raw > max_raw) val_raw = max_raw;

        uint64_t num = static_cast<uint64_t>(val_raw - min_raw) << 16;
        uint64_t t = num / static_cast<uint64_t>(span);
        if (t > 0xFFFFu) t = 0xFFFFu;
        return BoundedScalar(static_cast<uint16_t>(t));
    }
}
