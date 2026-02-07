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
#include "renderer/pipeline/ranges/LinearRange.h"

namespace PolarShader {
    namespace {
        constexpr int32_t BASE_CART = 200 << CARTESIAN_FRAC_BITS;
        constexpr CartQ24_8 BASE_WARP_SCALE = CartQ24_8(6 * BASE_CART);
        constexpr CartQ24_8 STRONG_WARP_SCALE = CartQ24_8(12 * BASE_CART);
        constexpr CartQ24_8 MAX_OFFSET_SOFT = CartQ24_8(12 * BASE_CART);
        constexpr CartQ24_8 MAX_OFFSET_MED = CartQ24_8(24 * BASE_CART);
        constexpr CartQ24_8 MAX_OFFSET_STRONG = CartQ24_8(36 * BASE_CART);
    }

    DomainWarpTransform domainWarpBasic() {
        return DomainWarpTransform(
            cPerMil(200),
            ceiling(),
            ceiling(),
            ceiling(),
            LinearRange(BASE_WARP_SCALE, BASE_WARP_SCALE),
            LinearRange(MAX_OFFSET_MED, MAX_OFFSET_MED)
        );
    }

    DomainWarpTransform domainWarpFbm(uint8_t octaves) {
        return DomainWarpTransform(
            DomainWarpTransform::WarpType::FBM,
            cPerMil(140),
            ceiling(),
            ceiling(),
            ceiling(),
            LinearRange(BASE_WARP_SCALE, BASE_WARP_SCALE),
            LinearRange(MAX_OFFSET_STRONG, MAX_OFFSET_STRONG),
            octaves
        );
    }

    DomainWarpTransform domainWarpNested() {
        return DomainWarpTransform(
            DomainWarpTransform::WarpType::Nested,
            cPerMil(100),
            ceiling(),
            ceiling(),
            ceiling(),
            LinearRange(BASE_WARP_SCALE, BASE_WARP_SCALE),
            LinearRange(MAX_OFFSET_STRONG, MAX_OFFSET_STRONG),
            2
        );
    }

    DomainWarpTransform domainWarpCurl() {
        return DomainWarpTransform(
            DomainWarpTransform::WarpType::Curl,
            cPerMil(260),
            ceiling(),
            ceiling(),
            ceiling(),
            LinearRange(BASE_WARP_SCALE, STRONG_WARP_SCALE),
            LinearRange(MAX_OFFSET_MED, MAX_OFFSET_MED),
            1
        );
    }

    DomainWarpTransform domainWarpPolar() {
        return DomainWarpTransform(
            DomainWarpTransform::WarpType::Polar,
            cPerMil(110),
            cPerMil(220),
            ceiling(),
            ceiling(),
            LinearRange(STRONG_WARP_SCALE, STRONG_WARP_SCALE),
            LinearRange(MAX_OFFSET_SOFT, MAX_OFFSET_SOFT),
            1
        );
    }

    DomainWarpTransform domainWarpDirectional() {
        return DomainWarpTransform(
            DomainWarpTransform::WarpType::Directional,
            cPerMil(130),
            cPerMil(240),
            ceiling(),
            ceiling(),
            LinearRange(BASE_WARP_SCALE, BASE_WARP_SCALE),
            LinearRange(MAX_OFFSET_MED, MAX_OFFSET_MED),
            1,
            noise(cPerMil(40)),
            cPerMil(180)
        );
    }
}
