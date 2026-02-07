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

#ifndef POLAR_SHADER_PIPELINE_MATHS_ZOOMMATHS_H
#define POLAR_SHADER_PIPELINE_MATHS_ZOOMMATHS_H

#include "renderer/pipeline/ranges/LinearRange.h"

namespace PolarShader {
    int32_t zoomMinScaleRaw(const LinearRange<SFracQ0_16> &range);

    int32_t zoomMaxScaleRaw(const LinearRange<SFracQ0_16> &range);

    SFracQ0_16 zoomNormalize(SFracQ0_16 value, int32_t min_scale_raw, int32_t max_scale_raw);
}

#endif // POLAR_SHADER_PIPELINE_MATHS_ZOOMMATHS_H
