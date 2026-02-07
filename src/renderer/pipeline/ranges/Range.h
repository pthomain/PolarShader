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

#include <utility>
#include "renderer/pipeline/signals/SignalTypes.h"

namespace PolarShader {
    template<typename Derived, typename T>
    class Range {
    public:
        using value_type = T;

        virtual ~Range() = default;

        virtual MappedValue<T> map(SFracQ0_16 t) const = 0;

        MappedSignal<T> mapSignal(SFracQ0_16Signal signal) const {
            Derived range_copy = static_cast<const Derived &>(*this);
            return MappedSignal<T>(
                [range_copy = std::move(range_copy),
                    signal = std::move(signal)](FracQ0_16 progress, TimeMillis elapsedMs) mutable {
                    (void) progress;
                    return range_copy.map(signal(elapsedMs));
                }
            );
        }
    };
}

#endif // POLAR_SHADER_PIPELINE_RANGES_RANGE_H
