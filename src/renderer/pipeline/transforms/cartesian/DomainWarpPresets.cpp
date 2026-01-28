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

#include "DomainWarpPresets.h"
#include "renderer/pipeline/units/UnitConstants.h"

namespace PolarShader {
    namespace {
        constexpr CartQ24_8 BASE_WARP_SCALE = CartQ24_8(1 << CARTESIAN_FRAC_BITS);
        constexpr CartQ24_8 STRONG_WARP_SCALE = CartQ24_8(2 << CARTESIAN_FRAC_BITS);
        constexpr CartQ24_8 MAX_OFFSET_SOFT = CartQ24_8(2 << CARTESIAN_FRAC_BITS);
        constexpr CartQ24_8 MAX_OFFSET_MED = CartQ24_8(4 << CARTESIAN_FRAC_BITS);
        constexpr CartQ24_8 MAX_OFFSET_STRONG = CartQ24_8(6 << CARTESIAN_FRAC_BITS);
    }

    DomainWarpTransform domainWarpBasic() {
        return DomainWarpTransform(
            noise(cPerMil(120)),
            cPerMil(250),
            BASE_WARP_SCALE,
            MAX_OFFSET_MED
        );
    }

    DomainWarpTransform domainWarpFbm(uint8_t octaves) {
        return DomainWarpTransform(
            DomainWarpTransform::WarpType::FBM,
            noise(cPerMil(140)),
            cPerMil(300),
            BASE_WARP_SCALE,
            MAX_OFFSET_STRONG,
            octaves,
            SFracQ0_16Signal(),
            SFracQ0_16Signal()
        );
    }

    DomainWarpTransform domainWarpNested() {
        return DomainWarpTransform(
            DomainWarpTransform::WarpType::Nested,
            noise(cPerMil(100)),
            cPerMil(280),
            BASE_WARP_SCALE,
            MAX_OFFSET_STRONG,
            2,
            SFracQ0_16Signal(),
            SFracQ0_16Signal()
        );
    }

    DomainWarpTransform domainWarpCurl() {
        return DomainWarpTransform(
            DomainWarpTransform::WarpType::Curl,
            noise(cPerMil(90)),
            cPerMil(260),
            BASE_WARP_SCALE,
            MAX_OFFSET_MED,
            1,
            SFracQ0_16Signal(),
            SFracQ0_16Signal()
        );
    }

    DomainWarpTransform domainWarpPolar() {
        return DomainWarpTransform(
            DomainWarpTransform::WarpType::Polar,
            noise(cPerMil(110)),
            cPerMil(220),
            STRONG_WARP_SCALE,
            MAX_OFFSET_SOFT,
            1,
            SFracQ0_16Signal(),
            SFracQ0_16Signal()
        );
    }

    DomainWarpTransform domainWarpDirectional() {
        return DomainWarpTransform(
            DomainWarpTransform::WarpType::Directional,
            noise(cPerMil(130)),
            cPerMil(240),
            BASE_WARP_SCALE,
            MAX_OFFSET_MED,
            1,
            noise(cPerMil(40)),
            cPerMil(180)
        );
    }
}
