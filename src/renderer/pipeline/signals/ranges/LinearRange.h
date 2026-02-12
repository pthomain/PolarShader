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

#include "renderer/pipeline/signals/ranges/Range.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include <algorithm>
#include <type_traits>
#include <utility>

namespace PolarShader {
    /**
     * @brief Generic linear range that maps signed signal samples into [min, max] for type T.
     *
     * Signed-domain behavior is configured by RangeMappingMode:
     * - SignedDirect: use signed signal values directly.
     * - UnsignedFromSigned: remap [-1, 1] -> [0, 1] first.
     * - Auto: choose SignedDirect when min<0, otherwise UnsignedFromSigned.
     */
    template<typename T>
    class LinearRange : public Range<T> {
    public:
        using Rep = typename std::decay<decltype(raw(std::declval<T>()))>::type;

        LinearRange(
            T minValue,
            T maxValue,
            RangeMappingMode mappingMode = RangeMappingMode::Auto
        ) : mode_(mappingMode) {
            min_raw = static_cast<int64_t>(raw(minValue));
            max_raw = static_cast<int64_t>(raw(maxValue));
            if (min_raw > max_raw) {
                std::swap(min_raw, max_raw);
            }
        }

        T map(SQ0_16 t) const override {
            int64_t span = max_raw - min_raw;
            if (span == 0) return T(static_cast<Rep>(min_raw));

            switch (resolvedMode()) {
                case RangeMappingMode::SignedDirect: {
                    constexpr int64_t signed_span = static_cast<int64_t>(SQ0_16_MAX) - static_cast<int64_t>(SQ0_16_MIN);
                    int64_t signed_raw = static_cast<int64_t>(Range<T>::mapSigned(t));
                    int64_t signed_t = signed_raw - static_cast<int64_t>(SQ0_16_MIN);
                    int64_t scaled = (span * signed_t + (signed_span / 2)) / signed_span;
                    return T(static_cast<Rep>(min_raw + scaled));
                }
                case RangeMappingMode::UnsignedFromSigned:
                default: {
                    uint32_t t_raw = Range<T>::mapUnsigned(t);
                    int64_t scaled = (span * static_cast<int64_t>(t_raw) + (1LL << 15)) >> 16;
                    return T(static_cast<Rep>(min_raw + scaled));
                }
            }
        }

        int64_t minRaw() const { return min_raw; }
        int64_t maxRaw() const { return max_raw; }

    private:
        RangeMappingMode resolvedMode() const {
            if (mode_ != RangeMappingMode::Auto) return mode_;
            return (min_raw < 0) ? RangeMappingMode::SignedDirect : RangeMappingMode::UnsignedFromSigned;
        }

        int64_t min_raw;
        int64_t max_raw;
        RangeMappingMode mode_;
    };
}

#endif // POLAR_SHADER_PIPELINE_RANGES_LINEARRANGE_H
