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

#ifndef POLAR_SHADER_PIPELINE_PIPELINECONTEXT_H
#define POLAR_SHADER_PIPELINE_PIPELINECONTEXT_H

#include "renderer/pipeline/units/ScalarUnits.h"
#include "renderer/pipeline/units/UnitConstants.h"

namespace PolarShader {
    struct PipelineContext {
        // Current zoom scale in Q0.16.
        SFracQ0_16 zoomScale = SFracQ0_16(Q0_16_ONE);
        // Normalized zoom scale in 0..1 (Q0.16), based on the zoom transform's range.
        SFracQ0_16 zoomNormalized = SFracQ0_16(Q0_16_ONE);
    };
}

#endif // POLAR_SHADER_PIPELINE_PIPELINECONTEXT_H
