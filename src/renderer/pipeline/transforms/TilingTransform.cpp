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

#include "TilingTransform.h"

namespace PolarShader {

    TilingTransform::TilingTransform(uint32_t tileX, uint32_t tileY)
        : tileX(tileX), tileY(tileY) {
    }

    CartesianLayer TilingTransform::operator()(const CartesianLayer &layer) const {
        return [tileX = this->tileX, tileY = this->tileY, layer](int32_t x, int32_t y) {
            auto tile_mod = [](int32_t v, uint32_t tile) -> int32_t {
                if (tile == 0) return v;
                int32_t m = v % static_cast<int32_t>(tile);
                if (m < 0) m += static_cast<int32_t>(tile);
                return m;
            };

            int32_t finalX = tile_mod(x, tileX);
            int32_t finalY = tile_mod(y, tileY);

            return layer(finalX, finalY);
        };
    }
}
