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

#ifndef POLAR_SHADER_TRANSFORMS_POLAR_KALEIDOSCOPETRANSFORM_H
#define POLAR_SHADER_TRANSFORMS_POLAR_KALEIDOSCOPETRANSFORM_H

#include "renderer/pipeline/transforms/base/Transforms.h"
#include <memory>

namespace PolarShader {
    /**
     * Polar kaleidoscope: folds angle into nbFacets wedges, with optional mirroring.
     */
    class KaleidoscopeTransform : public PolarTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        KaleidoscopeTransform(uint8_t nbFacets, bool isMirrored);

        PolarLayer operator()(const PolarLayer &layer) const override;
    };
}

#endif // POLAR_SHADER_TRANSFORMS_POLAR_KALEIDOSCOPETRANSFORM_H
