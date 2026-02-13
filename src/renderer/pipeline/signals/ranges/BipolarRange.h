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

#ifndef POLAR_SHADER_PIPELINE_RANGES_BIPOLARRANGE_H
#define POLAR_SHADER_PIPELINE_RANGES_BIPOLARRANGE_H

#include "renderer/pipeline/signals/ranges/Range.h"
#include <type_traits>
#include <utility>

namespace PolarShader {
    /**
     * @brief Signed bipolar range: maps signed signal [-1, 1] directly
     * (preserving sign) into [min, max].
     */
    template<typename T>
    class BipolarRange : public Range<T> {
    public:
        using Rep = typename std::decay<decltype(raw(std::declval<T>()))>::type;

        BipolarRange(T minValue, T maxValue) {
            min_raw = static_cast<int64_t>(raw(minValue));
            max_raw = static_cast<int64_t>(raw(maxValue));
            if (min_raw > max_raw) {
                std::swap(min_raw, max_raw);
            }
        }

        T map(sf16 t) const override {
            int64_t span = max_raw - min_raw;
            if (span == 0) return T(static_cast<Rep>(min_raw));

            constexpr int64_t signed_span = static_cast<int64_t>(SF16_MAX) - static_cast<int64_t>(SF16_MIN);
            int64_t signed_raw = static_cast<int64_t>(Range<T>::mapSigned(t));
            int64_t signed_t = signed_raw - static_cast<int64_t>(SF16_MIN);
            int64_t scaled = (span * signed_t + (signed_span / 2)) / signed_span;
            return T(static_cast<Rep>(min_raw + scaled));
        }

        int64_t minRaw() const { return min_raw; }
        int64_t maxRaw() const { return max_raw; }

    private:
        int64_t min_raw;
        int64_t max_raw;
    };
}

#endif // POLAR_SHADER_PIPELINE_RANGES_BIPOLARRANGE_H
