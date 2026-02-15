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

#include "renderer/pipeline/transforms/DomainWarpPresets.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"

namespace PolarShader {
    namespace {
        constexpr int32_t BASE_CART = 200 << R8_FRAC_BITS;
        constexpr sr8 BASE_WARP_SCALE = sr8(6 * BASE_CART);
        constexpr sr8 STRONG_WARP_SCALE = sr8(12 * BASE_CART);
        constexpr sr8 MAX_OFFSET_SOFT = sr8(12 * BASE_CART);
        constexpr sr8 MAX_OFFSET_MED = sr8(24 * BASE_CART);
        constexpr sr8 MAX_OFFSET_STRONG = sr8(36 * BASE_CART);
    }

    DomainWarpTransform domainWarpBasic() {
        return DomainWarpTransform(
            biMid(200),
            biCeil(),
            biCeil(),
            biCeil(),
            MagnitudeRange(BASE_WARP_SCALE, BASE_WARP_SCALE),
            MagnitudeRange(MAX_OFFSET_MED, MAX_OFFSET_MED)
        );
    }

    DomainWarpTransform domainWarpFbm(uint8_t octaves) {
        return DomainWarpTransform(
            DomainWarpTransform::WarpType::FBM,
            biMid(140),
            biCeil(),
            biCeil(),
            biCeil(),
            MagnitudeRange(BASE_WARP_SCALE, BASE_WARP_SCALE),
            MagnitudeRange(MAX_OFFSET_STRONG, MAX_OFFSET_STRONG),
            octaves
        );
    }

    DomainWarpTransform domainWarpNested() {
        return DomainWarpTransform(
            DomainWarpTransform::WarpType::Nested,
            biMid(100),
            biCeil(),
            biCeil(),
            biCeil(),
            MagnitudeRange(BASE_WARP_SCALE, BASE_WARP_SCALE),
            MagnitudeRange(MAX_OFFSET_STRONG, MAX_OFFSET_STRONG),
            2
        );
    }

    DomainWarpTransform domainWarpCurl() {
        return DomainWarpTransform(
            DomainWarpTransform::WarpType::Curl,
            biMid(260),
            biCeil(),
            biCeil(),
            biCeil(),
            MagnitudeRange(BASE_WARP_SCALE, STRONG_WARP_SCALE),
            MagnitudeRange(MAX_OFFSET_MED, MAX_OFFSET_MED),
            1
        );
    }

    DomainWarpTransform domainWarpPolar() {
        return DomainWarpTransform(
            DomainWarpTransform::WarpType::Polar,
            biMid(110),
            biMid(220),
            biCeil(),
            biCeil(),
            MagnitudeRange(STRONG_WARP_SCALE, STRONG_WARP_SCALE),
            MagnitudeRange(MAX_OFFSET_SOFT, MAX_OFFSET_SOFT),
            1
        );
    }

    DomainWarpTransform domainWarpDirectional() {
        return DomainWarpTransform(
            DomainWarpTransform::WarpType::Directional,
            biMid(130),
            biMid(240),
            biCeil(),
            biCeil(),
            MagnitudeRange(BASE_WARP_SCALE, BASE_WARP_SCALE),
            MagnitudeRange(MAX_OFFSET_MED, MAX_OFFSET_MED),
            1,
            noise(biMid(40)),
            biMid(180)
        );
    }
}
