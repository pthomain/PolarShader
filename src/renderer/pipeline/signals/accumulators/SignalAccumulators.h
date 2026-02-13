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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_SIGNAL_ACCUMULATORS_H
#define POLAR_SHADER_PIPELINE_SIGNALS_SIGNAL_ACCUMULATORS_H

#include "renderer/pipeline/signals/SignalTypes.h"
#include <limits>
#include <type_traits>

namespace PolarShader {
    namespace detail {
        template<typename T>
        struct SignalAccumulator {
            static T zero() { return T(0); }

            static T add(T base, T delta) {
                if constexpr (std::is_arithmetic_v<T>) {
                    return static_cast<T>(static_cast<int64_t>(base) + static_cast<int64_t>(delta));
                } else {
                    using Rep = std::decay_t<decltype(raw(base))>;
                    int64_t sum = static_cast<int64_t>(raw(base)) + static_cast<int64_t>(raw(delta));
                    if constexpr (std::numeric_limits<Rep>::is_integer) {
                        if constexpr (std::numeric_limits<Rep>::is_signed) {
                            if (sum > static_cast<int64_t>(std::numeric_limits<Rep>::max())) {
                                sum = static_cast<int64_t>(std::numeric_limits<Rep>::max());
                            }
                            if (sum < static_cast<int64_t>(std::numeric_limits<Rep>::min())) {
                                sum = static_cast<int64_t>(std::numeric_limits<Rep>::min());
                            }
                        } else {
                            if (sum < 0) sum = 0;
                            if (sum > static_cast<int64_t>(std::numeric_limits<Rep>::max())) {
                                sum = static_cast<int64_t>(std::numeric_limits<Rep>::max());
                            }
                        }
                    }
                    return T(static_cast<Rep>(sum));
                }
            }
        };

        template<>
        struct SignalAccumulator<f16> {
            static f16 zero() { return f16(0); }

            static f16 add(f16 base, f16 delta) {
                uint32_t sum = static_cast<uint32_t>(raw(base)) + static_cast<uint32_t>(raw(delta));
                return f16(static_cast<uint16_t>(sum));
            }
        };

        template<>
        struct SignalAccumulator<sf16> {
            static sf16 zero() { return sf16(0); }

            static sf16 add(sf16 base, sf16 delta) {
                int64_t sum = static_cast<int64_t>(raw(base)) + static_cast<int64_t>(raw(delta));
                return clampSf16Sat(sum);
            }
        };

        template<>
        struct SignalAccumulator<int32_t> {
            static int32_t zero() { return 0; }

            static int32_t add(int32_t base, int32_t delta) {
                int64_t sum = static_cast<int64_t>(base) + static_cast<int64_t>(delta);
                if (sum > std::numeric_limits<int32_t>::max()) sum = std::numeric_limits<int32_t>::max();
                if (sum < std::numeric_limits<int32_t>::min()) sum = std::numeric_limits<int32_t>::min();
                return static_cast<int32_t>(sum);
            }
        };

        template<>
        struct SignalAccumulator<uint8_t> {
            static uint8_t zero() { return 0; }

            static uint8_t add(uint8_t base, uint8_t delta) {
                return static_cast<uint8_t>(static_cast<uint16_t>(base) + static_cast<uint16_t>(delta));
            }
        };

        template<>
        struct SignalAccumulator<sr8> {
            static sr8 zero() { return sr8(0); }

            static sr8 add(sr8 base, sr8 delta) {
                int64_t sum = static_cast<int64_t>(raw(base)) + static_cast<int64_t>(raw(delta));
                if (sum > std::numeric_limits<int32_t>::max()) sum = std::numeric_limits<int32_t>::max();
                if (sum < std::numeric_limits<int32_t>::min()) sum = std::numeric_limits<int32_t>::min();
                return sr8(static_cast<int32_t>(sum));
            }
        };

        template<>
        struct SignalAccumulator<sr16> {
            static sr16 zero() { return sr16(0); }

            static sr16 add(sr16 base, sr16 delta) {
                return base + delta;
            }
        };

    }

}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_SIGNAL_ACCUMULATORS_H
