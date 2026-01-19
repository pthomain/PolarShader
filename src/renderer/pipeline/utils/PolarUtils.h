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

#ifndef POLAR_SHADER_PIPELINE_POLARUTILS_H
#define POLAR_SHADER_PIPELINE_POLARUTILS_H

#include "Units.h"

namespace PolarShader {
    struct CRGB16 {
        uint16_t r;
        uint16_t g;
        uint16_t b;

        CRGB16(uint16_t r = 0, uint16_t g = 0, uint16_t b = 0);

        CRGB16 &operator+=(const CRGB &rhs);
    };

    // Phase stores AngleUnitsQ0_16 in the high 16 bits; trig sampling uses (phase >> 16). Callers must
    // pass a promoted AngleTurnsUQ16_16; no auto-promotion happens here.
    fl::pair<int32_t, int32_t> cartesianCoords(
        AngleTurnsUQ16_16 angle_q16,
        RadiusQ0_16 radius
    );

    fl::pair<AngleTurnsUQ16_16, RadiusQ0_16> polarCoords(
        fl::i32 x,
        fl::i32 y
    );
}
#endif //POLAR_SHADER_PIPELINE_POLARUTILS_H
