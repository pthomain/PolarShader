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

#ifndef POLAR_SHADER_TRANSFORMS_POSTERIZEPOLARTRANSFORM_H
#define POLAR_SHADER_TRANSFORMS_POSTERIZEPOLARTRANSFORM_H

#include "base/Transforms.h"
#include "renderer/pipeline/utils/Units.h"

namespace PolarShader {
    /**
     * Quantizes polar coordinates into discrete bands: angle and/or radius are snapped to bins.
     * Useful for posterized rings/sectors before applying vortex/rotation.
     *
     * Parameters: angleBins (1..255, 0/1 disables), radiusBins (1..255, 0/1 disables).
     * Recommended order: early in polar chain, before vortex/kaleidoscope.
     */
    class PosterizePolarTransform : public PolarTransform {
        uint8_t angleBins;
        uint8_t radiusBins;

    public:
        PosterizePolarTransform(uint8_t angleBins, uint8_t radiusBins);

        PolarLayer operator()(const PolarLayer &layer) const override;
    };
}

#endif //POLAR_SHADER_TRANSFORMS_POSTERIZEPOLARTRANSFORM_H
