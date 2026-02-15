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

#ifndef POLAR_SHADER_PIPELINE_RANGES_MAGNITUDERANGE_H
#define POLAR_SHADER_PIPELINE_RANGES_MAGNITUDERANGE_H

#include "renderer/pipeline/signals/ranges/Range.h"
#include <type_traits>
#include <utility>

namespace PolarShader {
    /**
     * @brief Unsigned magnitude range: maps signed signal [-1, 1] via unsigned
     * remapping [0, 1], then scales into [min, max].
     */
    template<typename T>
    class MagnitudeRange : public Range<T> {
    public:
        using Rep = typename std::decay<decltype(raw(std::declval<T>()))>::type;

        MagnitudeRange(T minValue, T maxValue) {
            min_raw = static_cast<int64_t>(raw(minValue));
            max_raw = static_cast<int64_t>(raw(maxValue));
            if (min_raw > max_raw) {
                std::swap(min_raw, max_raw);
            }
        }

        T map(sf16 t) const override {
            int64_t span = max_raw - min_raw;
            if (span == 0) return T(static_cast<Rep>(min_raw));

            uint32_t t_raw = Range<T>::mapUnsignedClamped(t);
            int64_t scaled = (span * static_cast<int64_t>(t_raw) + (1LL << 15)) >> 16;
            return T(static_cast<Rep>(min_raw + scaled));
        }

        int64_t minRaw() const { return min_raw; }
        int64_t maxRaw() const { return max_raw; }

    private:
        int64_t min_raw;
        int64_t max_raw;
    };
}

#endif // POLAR_SHADER_PIPELINE_RANGES_MAGNITUDERANGE_H
