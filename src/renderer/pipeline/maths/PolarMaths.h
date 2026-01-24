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

#ifndef POLAR_SHADER_PIPELINE_MATHS_POLARMATHS_H
#define POLAR_SHADER_PIPELINE_MATHS_POLARMATHS_H

#include "FastLED.h"
#include "renderer/pipeline/units/AngleUnits.h"
#include "renderer/pipeline/units/ScalarUnits.h"

namespace PolarShader {
    // Phase stores angle units in the high 16 bits; trig sampling uses (phase >> 16). Callers must
    // pass a promoted phase value; no auto-promotion happens here.
    fl::pair<int32_t, int32_t> polarToCartesian(
        UnboundedAngle angle_phase,
        BoundedScalar radius
    );

    fl::pair<UnboundedAngle, BoundedScalar> cartesianToPolar(
        fl::i32 x,
        fl::i32 y
    );
}

#endif // POLAR_SHADER_PIPELINE_MATHS_POLARMATHS_H
