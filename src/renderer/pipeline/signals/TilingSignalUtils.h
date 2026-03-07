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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_TILINGSIGNALUTILS_H
#define POLAR_SHADER_PIPELINE_SIGNALS_TILINGSIGNALUTILS_H

#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/maths/TilingMaths.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"

namespace PolarShader {
    inline int32_t sampleTilingCellSizeRaw(const Sf16Signal &signal, TimeMillis elapsedMs, int32_t fallback) {
        if (!signal) {
            return TilingMaths::normalizeCellSizeRaw(fallback, TilingMaths::MIN_CELL_SIZE_RAW);
        }

        MagnitudeRange range(TilingMaths::MIN_CELL_SIZE_RAW, TilingMaths::MAX_CELL_SIZE_RAW);
        return TilingMaths::normalizeCellSizeRaw(signal.sample(range, elapsedMs), fallback);
    }
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_TILINGSIGNALUTILS_H
