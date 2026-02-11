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
#include <cstdint>
#include <limits>

namespace PolarShader {
    namespace detail {
        template<typename T>
        struct SignalAccumulator {
            static T zero() { return T(0); }

            static T add(T base, T delta) {
                return static_cast<T>(static_cast<int64_t>(base) + static_cast<int64_t>(delta));
            }
        };

        template<>
        struct SignalAccumulator<FracQ0_16> {
            static FracQ0_16 zero() { return FracQ0_16(0); }

            static FracQ0_16 add(FracQ0_16 base, FracQ0_16 delta) {
                uint32_t sum = static_cast<uint32_t>(raw(base)) + static_cast<uint32_t>(raw(delta));
                return FracQ0_16(static_cast<uint16_t>(sum));
            }
        };

        template<>
        struct SignalAccumulator<SFracQ0_16> {
            static SFracQ0_16 zero() { return SFracQ0_16(0); }

            static SFracQ0_16 add(SFracQ0_16 base, SFracQ0_16 delta) {
                int64_t sum = static_cast<int64_t>(raw(base)) + static_cast<int64_t>(raw(delta));
                return scalarClampQ0_16Raw(sum);
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
        struct SignalAccumulator<CartQ24_8> {
            static CartQ24_8 zero() { return CartQ24_8(0); }

            static CartQ24_8 add(CartQ24_8 base, CartQ24_8 delta) {
                int64_t sum = static_cast<int64_t>(raw(base)) + static_cast<int64_t>(raw(delta));
                if (sum > std::numeric_limits<int32_t>::max()) sum = std::numeric_limits<int32_t>::max();
                if (sum < std::numeric_limits<int32_t>::min()) sum = std::numeric_limits<int32_t>::min();
                return CartQ24_8(static_cast<int32_t>(sum));
            }
        };

        template<>
        struct SignalAccumulator<FracQ16_16> {
            static FracQ16_16 zero() { return FracQ16_16(0); }

            static FracQ16_16 add(FracQ16_16 base, FracQ16_16 delta) {
                return FracQ16_16(raw(base) + raw(delta));
            }
        };

    }

    inline UVSignal resolveMappedSignal(UVSignal signal) { return signal; }
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_SIGNAL_ACCUMULATORS_H
