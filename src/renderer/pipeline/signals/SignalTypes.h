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
#include <type_traits>
#include <utility>

namespace PolarShader {
    enum class SignalKind : uint8_t {
        PERIODIC,
        APERIODIC
    };

    enum class LoopMode : uint8_t {
        RESET
    };

    /**
     * @brief Time-indexed scalar signal saturated to signed Q0.16 [-1, 1].
     */
    class SFracQ0_16Signal {
    public:
        using WaveformFn = fl::function<SFracQ0_16(TimeMillis)>;

        SFracQ0_16Signal() = default;

        SFracQ0_16Signal(
            SignalKind kind,
            WaveformFn waveform
        ) : kind_(kind),
            waveformFn(std::move(waveform)) {
        }

        SFracQ0_16Signal(
            SignalKind kind,
            LoopMode loopMode,
            TimeMillis durationMs,
            WaveformFn waveform
        ) : kind_(kind),
            loopMode_(loopMode),
            durationMs_(durationMs),
            waveformFn(std::move(waveform)) {
        }

        static SFracQ0_16Signal periodic(WaveformFn waveform) {
            return SFracQ0_16Signal(SignalKind::PERIODIC, std::move(waveform));
        }

        static SFracQ0_16Signal aperiodic(TimeMillis durationMs, LoopMode loopMode, WaveformFn waveform) {
            return SFracQ0_16Signal(SignalKind::APERIODIC, loopMode, durationMs, std::move(waveform));
        }

        template<typename RangeT>
        auto sample(const RangeT &range, TimeMillis elapsedMs) const {
            if (!waveformFn) return range.map(SFracQ0_16(0));

            TimeMillis relativeTime = elapsedMs;
            if (kind_ == SignalKind::APERIODIC) {
                if (durationMs_ == 0) return range.map(SFracQ0_16(0));

                switch (loopMode_) {
                    case LoopMode::RESET:
                    default:
                        relativeTime = elapsedMs % durationMs_;
                        break;
                }
            }

            SFracQ0_16 value = scalarClampQ0_16Raw(raw(waveformFn(relativeTime)));
            return range.map(value);
        }

        SignalKind kind() const {
            return kind_;
        }

        LoopMode loopMode() const {
            return loopMode_;
        }

        TimeMillis duration() const {
            return durationMs_;
        }

        explicit operator bool() const {
            return static_cast<bool>(waveformFn);
        }

    private:
        SignalKind kind_{SignalKind::PERIODIC};
        LoopMode loopMode_{LoopMode::RESET};
        TimeMillis durationMs_{0};
        WaveformFn waveformFn;
    };

    class UVSignal {
    public:
        using SampleFn = fl::function<UV(FracQ0_16, TimeMillis)>;

        UVSignal() = default;

        explicit UVSignal(SampleFn sample)
            : sampleFn(std::move(sample)) {
        }

        template<typename Fn, typename std::enable_if_t<
            std::is_invocable_r_v<UV, Fn &, FracQ0_16, TimeMillis>, int> = 0>
        explicit UVSignal(Fn sample)
            : sampleFn(std::move(sample)) {
        }

        template<typename Fn, typename std::enable_if_t<
            !std::is_invocable_r_v<UV, Fn &, FracQ0_16, TimeMillis> &&
            std::is_invocable_r_v<UV, Fn &, FracQ0_16>, int> = 0>
        explicit UVSignal(Fn sample)
            : sampleFn([sample = std::move(sample)](FracQ0_16 progress, TimeMillis) mutable {
                  return sample(progress);
              }) {
        }

        UV sample(FracQ0_16 progress, TimeMillis elapsedMs) const {
            return sampleFn ? sampleFn(progress, elapsedMs) : UV();
        }

        UV sample(FracQ0_16 progress) const {
            return sample(progress, 0);
        }

        UV operator()(FracQ0_16 progress, TimeMillis elapsedMs) const {
            return sample(progress, elapsedMs);
        }

        UV operator()(FracQ0_16 progress) const {
            return sample(progress, 0);
        }

        explicit operator bool() const {
            return static_cast<bool>(sampleFn);
        }

    private:
        SampleFn sampleFn;
    };

}

#endif // POLAR_SHADER_PIPELINE_SIGNALS_SIGNAL_TYPES_H
