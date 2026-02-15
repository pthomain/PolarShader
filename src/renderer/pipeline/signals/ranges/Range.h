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

#ifndef POLAR_SHADER_PIPELINE_RANGES_RANGE_H
#define POLAR_SHADER_PIPELINE_RANGES_RANGE_H

#include "renderer/pipeline/maths/units/Units.h"
#include "renderer/pipeline/maths/ScalarMaths.h"

namespace PolarShader {
    template<typename T>
    class Range {
    public:
        virtual ~Range() = default;

        virtual T map(sf16 t) const = 0;

    protected:
        static constexpr uint32_t mapUnsigned(sf16 t) {
            return mapUnsignedClamped(t);
        }

        static constexpr uint32_t mapUnsignedClamped(sf16 t) {
            return raw(toUnsignedClamped(t));
        }

        static constexpr uint32_t mapUnsignedWrapped(sf16 t) {
            return raw(toUnsignedWrapped(t));
        }

        static constexpr int32_t mapSigned(sf16 t) {
            int32_t value = raw(t);
            if (value > SF16_MAX) return SF16_MAX;
            if (value < SF16_MIN) return SF16_MIN;
            return value;
        }
    };
}

#endif // POLAR_SHADER_PIPELINE_RANGES_RANGE_H
