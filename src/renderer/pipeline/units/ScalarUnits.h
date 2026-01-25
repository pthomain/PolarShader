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

#ifndef POLAR_SHADER_PIPELINE_UNITS_SCALARUNITS_H
#define POLAR_SHADER_PIPELINE_UNITS_SCALARUNITS_H

#include <cstdint>
#include "StrongTypes.h"

namespace PolarShader {
    struct FracQ0_16_Tag {
    };

    struct SFracQ0_16_Tag {
    };

    /**
    *   An unsigned fraction in Q0.16, typically interpreted as [0, 1).
    */
    using FracQ0_16 = Strong<uint16_t, FracQ0_16_Tag>;

    /**
    *   Signed Q0.16 fixed-point scalar stored in a 32-bit raw value.
    */
    using SFracQ0_16 = Strong<int32_t, SFracQ0_16_Tag>;

    // --- Raw extractors ---
    constexpr uint16_t raw(FracQ0_16 f) { return f.raw(); }
    constexpr int32_t raw(SFracQ0_16 v) { return v.raw(); }
}

#endif // POLAR_SHADER_PIPELINE_UNITS_SCALARUNITS_H
