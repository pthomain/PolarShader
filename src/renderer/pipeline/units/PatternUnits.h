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

#ifndef POLAR_SHADER_PIPELINE_UNITS_PATTERNUNITS_H
#define POLAR_SHADER_PIPELINE_UNITS_PATTERNUNITS_H

#include <cstdint>
#include "StrongTypes.h"

namespace PolarShader {
    struct NoiseRawU16_Tag {
    };

    struct PatternNormU16_Tag {
    };

    /**
    *   The raw 16-bit output of FastLED's inoise16.
    */
    using NoiseRawU16 = Strong<uint16_t, NoiseRawU16_Tag>;

    /**
    *   A 16-bit value intended to represent a normalized pattern intensity in the full 0..65535 domain.
    */
    using PatternNormU16 = Strong<uint16_t, PatternNormU16_Tag>;

    // --- Raw extractors ---
    constexpr uint16_t raw(NoiseRawU16 n) { return n.raw(); }
    constexpr uint16_t raw(PatternNormU16 n) { return n.raw(); }
}

#endif // POLAR_SHADER_PIPELINE_UNITS_PATTERNUNITS_H
