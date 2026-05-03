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

#include "renderer/pipeline/patterns/FlowFieldPattern.h"
#include "renderer/pipeline/patterns/GridUtils.h"
#include "renderer/pipeline/signals/ranges/BipolarRange.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"
#include "ProfileRanges.h"
#include "HalfLifeRange.h"
#include <algorithm>

namespace PolarShader {
    using namespace grid;

    namespace {
        constexpr uint8_t kMinDotCount = 1;
        constexpr uint8_t kMaxDotCount = 8;

        // Orbital dot parameters.
        constexpr fl::s16x16 kMaxOrbitSpeed = s16x16FromFraction(1, 4);
        constexpr fl::s16x16 kMinOrbitRadius = s16x16FromFraction(1, 8);
        constexpr fl::s16x16 kMaxOrbitRadius = s16x16FromFraction(15, 32); // 0.46875

        // NoisePunch decay half-life in milliseconds.
        constexpr uint16_t kPunchDecayHalfLifeMs = 1500u;

        const MagnitudeRange<fl::s16x16> &orbitSpeedRange() {
            static const MagnitudeRange range(s16x16FromFraction(0, 1), kMaxOrbitSpeed);
            return range;
        }

        const MagnitudeRange<fl::s16x16> &orbitRadiusRange() {
            static const MagnitudeRange range(kMinOrbitRadius, kMaxOrbitRadius);
            return range;
        }

        // ---- NoisePunch ----

        struct NoisePunch {
            fl::s16x16 center{fl::s16x16::from_raw(0)};
            fl::s16x16 width{fl::s16x16::from_raw(0)};
            sf16 amplitude{sf16(0)};
            FlowFieldPattern::NoisePunchShape shape{FlowFieldPattern::NoisePunchShape::HalfSine};

            void trigger(fl::s16x16 c, fl::s16x16 w, sf16 amp, FlowFieldPattern::NoisePunchShape s) {
                center = c;
                width = w;
                amplitude = amp;
                shape = s;
            }

            void decay(TimeMillis dtMs) {
                if (raw(amplitude) == 0) return;
                f16 factor = halfLifeFade(dtMs, kPunchDecayHalfLifeMs);
                int32_t decayed = (static_cast<int64_t>(raw(amplitude)) * raw(factor)) >> 16;
                // Kill small residuals.
                if (decayed < 16 && decayed > -16) decayed = 0;
                amplitude = sf16(static_cast<int32_t>(decayed));
            }

            sf16 evaluate(uint8_t index, uint8_t gridSize) const {
                if (raw(amplitude) == 0 || raw(width) == 0) return sf16(0);

                // Compute position of index relative to center, normalised to [-1, 1] over width.
                int32_t posRaw = (static_cast<int32_t>(index) << kQ16Shift) + (SF16_ONE >> 1);
                int32_t halfWidthRaw = raw(width) >> 1;
                if (halfWidthRaw == 0) halfWidthRaw = 1;
                int32_t deltaRaw = posRaw - raw(center);
                // x in [-1, 1] as Q16.16
                int32_t xRaw = (static_cast<int64_t>(deltaRaw) << 16) / halfWidthRaw;
                if (xRaw < -(1 << 16) || xRaw > (1 << 16)) return sf16(0);

                int32_t shapedRaw;
                if (shape == FlowFieldPattern::NoisePunchShape::HalfSine) {
                    // Map x from [-1, 1] to angle [0, pi] -> f16 [0, 0x8000].
                    uint16_t angle = static_cast<uint16_t>(((xRaw + SF16_ONE) * 0x8000LL) >> 16);
                    shapedRaw = raw(angleSinF16(f16(angle)));
                } else {
                    // Gaussian approximation: (1 - x^2)^2
                    int32_t x2 = static_cast<int32_t>((static_cast<int64_t>(xRaw) * xRaw) >> 16);
                    int32_t oneMinusX2 = SF16_ONE - x2;
                    if (oneMinusX2 < 0) return sf16(0);
                    shapedRaw = static_cast<int32_t>((static_cast<int64_t>(oneMinusX2) * oneMinusX2) >> 16);
                }

                // Scale by amplitude.
                int32_t result = static_cast<int32_t>((static_cast<int64_t>(shapedRaw) * raw(amplitude)) >> 16);
                return sf16(result);
            }
        };

        // ---- Orbital dot state ----

        struct OrbitalDot {
            uint16_t initialPhase; // f16-scale angle offset, fixed at construction
        };
    }

    // ---- State ----

    struct FlowFieldPattern::State {
        uint8_t gridSize;
        uint8_t dotCount;
        EmitterMode mode;
        std::unique_ptr<uint16_t[]> cells;
        std::unique_ptr<uint16_t[]> rowPass;
        std::unique_ptr<sf16[]> columnShiftProfile;
        std::unique_ptr<sf16[]> rowShiftProfile;
        uint32_t xProfileNoiseSeed;
        uint32_t yProfileNoiseSeed;
        Sf16Signal xDriftSignal;
        Sf16Signal xAmplitudeSignal;
        Sf16Signal xFrequencySignal;
        Sf16Signal yDriftSignal;
        Sf16Signal yAmplitudeSignal;
        Sf16Signal yFrequencySignal;
        Sf16Signal endpointSpeedSignal;
        Sf16Signal halfLifeSignal;
        Sf16Signal orbitSpeedSignal;
        Sf16Signal orbitRadiusSignal;
        fl::s16x16 xDrift{fl::s16x16::from_raw(0)};
        f16 xAmplitude{f16(0)};
        fl::s16x16 xFrequency{fl::s16x16::from_raw(0)};
        fl::s16x16 yDrift{fl::s16x16::from_raw(0)};
        f16 yAmplitude{f16(0)};
        fl::s16x16 yFrequency{fl::s16x16::from_raw(0)};
        fl::s16x16 endpointSpeed{fl::s16x16::from_raw(0)};
        uint16_t halfLifeMs{860u};
        fl::s16x16 orbitSpeed{fl::s16x16::from_raw(0)};
        fl::s16x16 orbitRadius{fl::s16x16::from_raw(0)};
        fl::s16x16 timeQ16{fl::s16x16::from_raw(0)};
        TimeMillis lastElapsedMs{0u};
        bool hasLastElapsed{false};
        NoisePunch punchX;
        NoisePunch punchY;
        OrbitalDot dots[8];

        explicit State(
            uint8_t size,
            uint8_t dots,
            EmitterMode emitterMode,
            Sf16Signal xDrift,
            Sf16Signal yDrift,
            Sf16Signal amplitude,
            Sf16Signal frequency,
            Sf16Signal endpointSpeed,
            Sf16Signal halfLife,
            Sf16Signal orbitSpeed,
            Sf16Signal orbitRadius
        ) : gridSize(size),
            dotCount(dots),
            mode(emitterMode),
            cells(std::make_unique<uint16_t[]>(static_cast<size_t>(size) * size)),
            rowPass(std::make_unique<uint16_t[]>(static_cast<size_t>(size) * size)),
            columnShiftProfile(std::make_unique<sf16[]>(size)),
            rowShiftProfile(std::make_unique<sf16[]>(size)),
            xProfileNoiseSeed(random32()),
            yProfileNoiseSeed(random32()),
            xDriftSignal(std::move(xDrift)),
            xAmplitudeSignal(amplitude),
            xFrequencySignal(frequency),
            yDriftSignal(std::move(yDrift)),
            yAmplitudeSignal(std::move(amplitude)),
            yFrequencySignal(std::move(frequency)),
            endpointSpeedSignal(std::move(endpointSpeed)),
            halfLifeSignal(std::move(halfLife)),
            orbitSpeedSignal(std::move(orbitSpeed)),
            orbitRadiusSignal(std::move(orbitRadius)) {
            std::fill_n(cells.get(), static_cast<size_t>(size) * size, uint16_t(0));
            std::fill_n(rowPass.get(), static_cast<size_t>(size) * size, uint16_t(0));

            // Initialise orbital dots with evenly-spaced initial phases + random jitter.
            uint16_t phaseStep = F16_MAX / static_cast<uint16_t>(dotCount);
            for (uint8_t i = 0; i < dotCount; ++i) {
                uint16_t jitter = static_cast<uint16_t>(random32() & 0x1FFFu); // small jitter
                this->dots[i].initialPhase = static_cast<uint16_t>(i * phaseStep + jitter);
            }
        }
    };

    // ---- Functor ----

    struct FlowFieldPattern::FlowFieldFunctor {
        const State *state;

        PatternNormU16 operator()(UV uv) const {
            return sampleScalarGridClamped(state->cells.get(), state->gridSize, state->gridSize, uv);
        }
    };

    // ---- Constructor ----

    FlowFieldPattern::FlowFieldPattern(
        uint8_t gridSize,
        uint8_t dotCount,
        EmitterMode mode,
        Sf16Signal xDrift,
        Sf16Signal yDrift,
        Sf16Signal amplitude,
        Sf16Signal frequency,
        Sf16Signal endpointSpeed,
        Sf16Signal halfLife,
        Sf16Signal orbitSpeed,
        Sf16Signal orbitRadius
    ) : state(std::make_shared<State>(
        std::max<uint8_t>(kMinGridSize, std::min<uint8_t>(gridSize, kMaxGridSize)),
        std::max<uint8_t>(kMinDotCount, std::min<uint8_t>(dotCount, kMaxDotCount)),
        mode,
        std::move(xDrift),
        std::move(yDrift),
        std::move(amplitude),
        std::move(frequency),
        std::move(endpointSpeed),
        std::move(halfLife),
        std::move(orbitSpeed),
        std::move(orbitRadius)
    )) {
    }

    // ---- NoisePunch triggers ----

    void FlowFieldPattern::triggerNoisePunchX(
        fl::s16x16 center, fl::s16x16 width, sf16 amplitude, NoisePunchShape shape
    ) {
        state->punchX.trigger(center, width, amplitude, shape);
    }

    void FlowFieldPattern::triggerNoisePunchY(
        fl::s16x16 center, fl::s16x16 width, sf16 amplitude, NoisePunchShape shape
    ) {
        state->punchY.trigger(center, width, amplitude, shape);
    }

    // ---- advanceFrame ----

    void FlowFieldPattern::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void)progress;
        State &s = *state;

        // 1. Compute delta time and accumulate timeQ16.
        TimeMillis dtMs = 0u;
        if (!s.hasLastElapsed) {
            s.lastElapsedMs = elapsedMs;
            s.hasLastElapsed = true;
        } else {
            dtMs = clampDeltaTime(elapsedMs - s.lastElapsedMs);
            s.lastElapsedMs = elapsedMs;
            s.timeQ16 = s.timeQ16 + fl::s16x16::from_raw(raw(timeMillisToScalar(dtMs)));
        }

        // 2. Sample all signals.
        s.xDrift = s.xDriftSignal.sample(profileSpeedRange(), elapsedMs);
        s.yDrift = s.yDriftSignal.sample(profileSpeedRange(), elapsedMs);
        s.xAmplitude = s.xAmplitudeSignal.sample(profileAmplitudeRange(), elapsedMs);
        s.yAmplitude = s.yAmplitudeSignal.sample(profileAmplitudeRange(), elapsedMs);
        s.xFrequency = s.xFrequencySignal.sample(profileFrequencyRange(), elapsedMs);
        s.yFrequency = s.yFrequencySignal.sample(profileFrequencyRange(), elapsedMs);
        s.endpointSpeed = s.endpointSpeedSignal.sample(endpointSpeedRange(), elapsedMs);
        s.orbitSpeed = s.orbitSpeedSignal.sample(orbitSpeedRange(), elapsedMs);
        s.orbitRadius = s.orbitRadiusSignal.sample(orbitRadiusRange(), elapsedMs);

        // Sample half-life: map magnitude signal [0..1] to [kHalfLifeMinMs, kHalfLifeMaxMs].
        {
            uint16_t halfLifeT = raw(s.halfLifeSignal.sample(magnitudeRange(), elapsedMs));
            uint32_t range = kHalfLifeMaxMs - kHalfLifeMinMs;
            s.halfLifeMs = static_cast<uint16_t>(kHalfLifeMinMs + ((static_cast<uint32_t>(halfLifeT) * range) >> 16));
        }

        // 3. Decay noise punch amplitudes.
        if (dtMs > 0u) {
            s.punchX.decay(dtMs);
            s.punchY.decay(dtMs);
        }

        // 4. Build noise profiles (with punch bias).
        uint8_t gridSize = s.gridSize;
        fl::s16x16 xPhase = s.timeQ16 * s.xDrift;
        fl::s16x16 yPhase = s.timeQ16 * s.yDrift;
        for (uint8_t index = 0; index < gridSize; ++index) {
            fl::s16x16 profileCoord = fl::s16x16::from_raw(static_cast<int32_t>(index) * raw(kBaseNoiseFrequency));
            fl::s16x16 xProfileCoord = profileCoord * s.xFrequency;
            fl::s16x16 yProfileCoord = profileCoord * s.yFrequency;

            sf16 columnShift = scaleSf16(
                sampleProfileNoise(xProfileCoord + xPhase, s.xProfileNoiseSeed),
                s.xAmplitude
            );
            sf16 rowShift = scaleSf16(
                sampleProfileNoise(yProfileCoord + yPhase, s.yProfileNoiseSeed),
                s.yAmplitude
            );

            // Add noise punch bias.
            sf16 punchBiasX = s.punchX.evaluate(index, gridSize);
            sf16 punchBiasY = s.punchY.evaluate(index, gridSize);
            columnShift = sf16(raw(columnShift) + raw(punchBiasX));
            rowShift = sf16(raw(rowShift) + raw(punchBiasY));

            s.columnShiftProfile[gridSize - 1u - index] = columnShift;
            s.rowShiftProfile[index] = rowShift;
        }

        // 5. Inject emitters.
        uint16_t *cells = s.cells.get();
        fl::s16x16 gridCenter = fl::s16x16::from_raw(static_cast<int32_t>(gridSize - 1u) * raw(kGridHalf));

        // Lissajous line.
        if (s.mode == EmitterMode::Lissajous || s.mode == EmitterMode::Both) {
            emitLissajousLine(cells, gridSize, gridCenter, s.timeQ16, s.endpointSpeed);
        }

        // Orbital dots.
        if (s.mode == EmitterMode::Dots || s.mode == EmitterMode::Both) {
            f16 dotIntensity(clampU16(std::max<uint32_t>(1u, kFullIntensity / static_cast<uint32_t>(s.dotCount))));
            fl::s16x16 radiusCells = scaleByGridSize(gridSize, s.orbitRadius);

            for (uint8_t i = 0; i < s.dotCount; ++i) {
                // Compute absolute phase from time (like Lissajous endpoints).
                // Per-dot rate variation (1 + 1/2^(3+i)) for visual interest.
                fl::s16x16 dotRate = s.orbitSpeed + (s.orbitSpeed >> (3 + i));
                uint16_t absolutePhase = static_cast<uint16_t>(
                    raw(s.timeQ16 * dotRate) + s.dots[i].initialPhase
                );

                f16 angle(absolutePhase);
                fl::s16x16 dotX = gridCenter + mulS16x16(radiusCells, angleCosF16(angle));
                fl::s16x16 dotY = gridCenter + mulS16x16(radiusCells, angleSinF16(angle));
                drawSubpixelPoint(cells, gridSize, dotX, dotY, dotIntensity);
            }
        }

        // 6. Advect: row-pass then column-pass.
        uint16_t *rowPass = s.rowPass.get();

        for (uint8_t y = 0; y < gridSize; ++y) {
            fl::s16x16 rowShift = mulS16x16(kRowShiftPixels, s.rowShiftProfile[y]);
            size_t rowBaseIndex = static_cast<size_t>(y) * gridSize;
            for (uint8_t x = 0; x < gridSize; ++x) {
                rowPass[rowBaseIndex + x] = sampleShiftedLaneWrapped(
                    cells, rowBaseIndex, 1u, gridSize, x, rowShift
                );
            }
        }

        // 7. Column-pass + fade using half-life.
        f16 fadeFactor = halfLifeFade(dtMs, s.halfLifeMs);
        for (uint8_t x = 0; x < gridSize; ++x) {
            fl::s16x16 columnShift = mulS16x16(kColShiftPixels, s.columnShiftProfile[x]);
            for (uint8_t y = 0; y < gridSize; ++y) {
                uint16_t advected = sampleShiftedLaneWrapped(
                    rowPass, x, gridSize, gridSize, y, columnShift
                );
                cells[static_cast<size_t>(y) * gridSize + x] = scaleU16ByF16(advected, fadeFactor);
            }
        }
    }

    // ---- layer ----

    UVMap FlowFieldPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        (void)context;
        return FlowFieldFunctor{state.get()};
    }
}
