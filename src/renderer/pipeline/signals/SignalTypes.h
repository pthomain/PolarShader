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

#ifndef POLAR_SHADER_PIPELINE_SIGNALS_SIGNAL_TYPES_H
#define POLAR_SHADER_PIPELINE_SIGNALS_SIGNAL_TYPES_H

#ifdef ARDUINO
#include "FastLED.h"
#else
#include "native/FastLED.h"
#endif
#include "renderer/pipeline/maths/units/Units.h"
#include "renderer/pipeline/maths/ScalarMaths.h"
#include "renderer/pipeline/maths/UVMaths.h"
#include <cstdint>
#include <limits>
#include <utility>

namespace PolarShader {
    /**
     * @brief Time-indexed scalar signal bounded to Q0.16.
     *
     * Use when:
     * - The consumer maps the output into its own min/max range.
     */
    class SFracQ0_16Signal {
    public:
        using SampleFn = fl::function<SFracQ0_16(TimeMillis)>;

        SFracQ0_16Signal() = default;

        SFracQ0_16Signal(SampleFn sample, bool absolute = true)
            : sampleFn(std::move(sample)),
              absoluteMode(absolute) {
        }

        SFracQ0_16 sample(TimeMillis time) const {
            return sampleFn ? sampleFn(time) : SFracQ0_16(0);
        }

        bool isAbsolute() const {
            return absoluteMode;
        }

        SFracQ0_16 operator()(TimeMillis time) const {
            return sample(time);
        }

        explicit operator bool() const {
            return static_cast<bool>(sampleFn);
        }

        SFracQ0_16Signal withAbsolute(bool absolute) const {
            SFracQ0_16Signal copy(*this);
            copy.absoluteMode = absolute;
            return copy;
        }

    private:
        SampleFn sampleFn;
        bool absoluteMode{true};
    };

    /**
     * @brief Time-indexed signal that emits mapped values.
     */
    template<typename T>
    class MappedSignal {
    public:
        using SampleFn = fl::function<MappedValue<T>(TimeMillis)>;

        MappedSignal() = default;

        MappedSignal(SampleFn sample, bool absolute = true)
            : sampleFn(std::move(sample)),
              absoluteMode(absolute) {
        }

        MappedValue<T> operator()(TimeMillis time) const {
            return sampleFn ? sampleFn(time) : MappedValue<T>();
        }

        bool isAbsolute() const {
            return absoluteMode;
        }

        explicit operator bool() const {
            return static_cast<bool>(sampleFn);
        }

        MappedSignal withAbsolute(bool absolute) const {
            MappedSignal copy(*this);
            copy.absoluteMode = absolute;
            return copy;
        }

    private:
        SampleFn sampleFn;
        bool absoluteMode{true};
    };

    /**
     * @brief Time-indexed signal that emits unified UV coordinates.
     */
    using UVSignal = MappedSignal<UV>;

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

        template<>
        struct SignalAccumulator<UV> {
            static UV zero() { return UV(FracQ16_16(0), FracQ16_16(0)); }
            static UV add(UV base, UV delta) {
                return UVMaths::add(base, delta);
            }
        };
    }

    template<typename T>
    MappedSignal<T> resolveMappedSignal(MappedSignal<T> signal) {
        if (!signal || signal.isAbsolute()) return signal;
        return MappedSignal<T>(
            [signal = std::move(signal),
                    accumulated = detail::SignalAccumulator<T>::zero()](TimeMillis time) mutable {
                MappedValue<T> value = signal(time);
                accumulated = detail::SignalAccumulator<T>::add(accumulated, value.get());
                return MappedValue<T>(accumulated);
            },
            true
        );
    }
}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_SIGNAL_TYPES_H
