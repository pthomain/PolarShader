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

#include "renderer/pipeline/maths/NoiseMaths.h"
#include "FastLED.h"

namespace PolarShader {
    NoiseRawU16 sampleNoiseBilinear(uint32_t x, uint32_t y) {
        // FastLED's inoise16 samples integer lattice points; we interpolate between
        // grid corners to support smooth Q24.8 coordinates without blocky stepping.
        uint32_t x_int = x >> CARTESIAN_FRAC_BITS;
        uint32_t y_int = y >> CARTESIAN_FRAC_BITS;
        uint32_t frac_mask = (1u << CARTESIAN_FRAC_BITS) - 1u;
        uint16_t x_frac = static_cast<uint16_t>((x & frac_mask) << (16 - CARTESIAN_FRAC_BITS));
        uint16_t y_frac = static_cast<uint16_t>((y & frac_mask) << (16 - CARTESIAN_FRAC_BITS));

        uint16_t n00 = inoise16(x_int, y_int);
        uint16_t n10 = inoise16(x_int + 1u, y_int);
        uint16_t n01 = inoise16(x_int, y_int + 1u);
        uint16_t n11 = inoise16(x_int + 1u, y_int + 1u);

        int32_t nx0 = static_cast<int32_t>(n00) +
                      ((static_cast<int32_t>(n10) - static_cast<int32_t>(n00)) * x_frac >> 16);
        int32_t nx1 = static_cast<int32_t>(n01) +
                      ((static_cast<int32_t>(n11) - static_cast<int32_t>(n01)) * x_frac >> 16);
        int32_t nxy = nx0 + ((nx1 - nx0) * y_frac >> 16);
        if (nxy < 0) nxy = 0;
        if (nxy > UINT16_MAX) nxy = UINT16_MAX;
        return NoiseRawU16(static_cast<uint16_t>(nxy));
    }

    NoiseRawU16 sampleNoiseTrilinear(uint32_t x, uint32_t y, uint32_t z) {
        // 3D variant of the same idea: trilinear interpolation over 8 corners
        // so animated depth (z) and fractional x/y produce continuous noise.
        uint32_t x_int = x >> CARTESIAN_FRAC_BITS;
        uint32_t y_int = y >> CARTESIAN_FRAC_BITS;
        uint32_t z_int = z >> CARTESIAN_FRAC_BITS;
        uint32_t frac_mask = (1u << CARTESIAN_FRAC_BITS) - 1u;
        uint16_t x_frac = static_cast<uint16_t>((x & frac_mask) << (16 - CARTESIAN_FRAC_BITS));
        uint16_t y_frac = static_cast<uint16_t>((y & frac_mask) << (16 - CARTESIAN_FRAC_BITS));
        uint16_t z_frac = static_cast<uint16_t>((z & frac_mask) << (16 - CARTESIAN_FRAC_BITS));

        uint16_t n000 = inoise16(x_int, y_int, z_int);
        uint16_t n100 = inoise16(x_int + 1u, y_int, z_int);
        uint16_t n010 = inoise16(x_int, y_int + 1u, z_int);
        uint16_t n110 = inoise16(x_int + 1u, y_int + 1u, z_int);
        uint16_t n001 = inoise16(x_int, y_int, z_int + 1u);
        uint16_t n101 = inoise16(x_int + 1u, y_int, z_int + 1u);
        uint16_t n011 = inoise16(x_int, y_int + 1u, z_int + 1u);
        uint16_t n111 = inoise16(x_int + 1u, y_int + 1u, z_int + 1u);

        int32_t nx00 = static_cast<int32_t>(n000) +
                       ((static_cast<int32_t>(n100) - static_cast<int32_t>(n000)) * x_frac >> 16);
        int32_t nx10 = static_cast<int32_t>(n010) +
                       ((static_cast<int32_t>(n110) - static_cast<int32_t>(n010)) * x_frac >> 16);
        int32_t nx01 = static_cast<int32_t>(n001) +
                       ((static_cast<int32_t>(n101) - static_cast<int32_t>(n001)) * x_frac >> 16);
        int32_t nx11 = static_cast<int32_t>(n011) +
                       ((static_cast<int32_t>(n111) - static_cast<int32_t>(n011)) * x_frac >> 16);

        int32_t nxy0 = nx00 + ((nx10 - nx00) * y_frac >> 16);
        int32_t nxy1 = nx01 + ((nx11 - nx01) * y_frac >> 16);
        int32_t nxyz = nxy0 + ((nxy1 - nxy0) * z_frac >> 16);
        if (nxyz < 0) nxyz = 0;
        if (nxyz > UINT16_MAX) nxyz = UINT16_MAX;
        return NoiseRawU16(static_cast<uint16_t>(nxyz));
    }
}
