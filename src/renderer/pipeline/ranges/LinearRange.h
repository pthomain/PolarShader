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

#ifndef POLAR_SHADER_PIPELINE_RANGES_LINEARRANGE_H
#define POLAR_SHADER_PIPELINE_RANGES_LINEARRANGE_H

#include "renderer/pipeline/ranges/Range.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include <utility>
#include <algorithm>

namespace PolarShader {
    namespace detail {
        template<typename T, typename = void>
        struct rep_type_trait {
            using type = T;
        };

        template<typename T>
        struct rep_type_trait<T, std::void_t<typename T::rep_type>> {
            using type = typename T::rep_type;
        };
    }

    /**
     * @brief Generic linear range that maps a 0..1 signal into a [min, max] range of type T.
     * 
     * Handles any type T that can be converted to/from an integer via raw() and constructor.
     */
    template<typename T>
    class LinearRange : public Range<LinearRange<T>, T> {
    public:
        using Rep = typename detail::rep_type_trait<T>::type;

        LinearRange(T minValue, T maxValue) {
            min_raw = static_cast<int64_t>(raw(minValue));
            max_raw = static_cast<int64_t>(raw(maxValue));
            if (min_raw > max_raw) {
                std::swap(min_raw, max_raw);
            }
        }

        MappedValue<T> map(SFracQ0_16 t) const override {
            int64_t span = max_raw - min_raw;
            if (span == 0) return MappedValue<T>(T(static_cast<Rep>(min_raw)));

            uint32_t t_raw = clamp_frac_raw(raw(t));
            int64_t scaled = (span * static_cast<int64_t>(t_raw) + (1LL << 15)) >> 16;
            
            #ifndef ARDUINO
            // printf("LinearRange::map: t=%u, min=%lld, max=%lld, span=%lld, scaled=%lld, result=%lld\n", t_raw, min_raw, max_raw, span, scaled, min_raw + scaled);
            #endif

            return MappedValue<T>(T(static_cast<Rep>(min_raw + scaled)));
        }

        int32_t minRaw() const { return static_cast<int32_t>(min_raw); }
        int32_t maxRaw() const { return static_cast<int32_t>(max_raw); }

    private:
        int64_t min_raw;
        int64_t max_raw;
    };
}

#endif // POLAR_SHADER_PIPELINE_RANGES_LINEARRANGE_H
