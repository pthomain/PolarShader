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

#include "renderer/pipeline/patterns/FlurryPattern.h"
#include "renderer/pipeline/patterns/GridUtils.h"
#include "renderer/pipeline/signals/ranges/BipolarRange.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"
#include <algorithm>

namespace PolarShader {
    using namespace grid;

    namespace {
        constexpr uint8_t kMinLineCount = 1;
        constexpr uint8_t kMaxLineCount = 8;

        constexpr fl::s16x16 kMinFrequency = s16x16FromFraction(1, 64); // 0.015625 (1/64)

        constexpr f16 kFadeBase = perMil(900); // 0.9
        constexpr f16 kFadeSpan = perMil(99); // 0.99 - avoid overflow

        constexpr f16 kMaxProfileAmplitude = toF16(1, 2); // ~1.0
        constexpr fl::s16x16 kMaxProfileSpeed = s16x16FromFraction(1, 4); // 0.0625 (1/16)
        constexpr fl::s16x16 kMaxProfileFrequency = s16x16FromFraction(1, 2);

        constexpr fl::s16x16 kBallDiscRadius = s16x16FromMixed(1, 1, 4); // 1.25
        constexpr fl::s16x16 kBallDiscSoftEdge = s16x16FromFraction(3, 4); // 0.75
        constexpr fl::s16x16 kBallRadiusSpread = s16x16FromFraction(3, 20); // 0.15

        constexpr uint16_t kEndpointAXPhase = 2086u; // 0.03183 turns
        constexpr uint16_t kEndpointAYPhase = 13557u; // 0.20686 turns
        constexpr uint16_t kEndpointBXPhase = 22938u; // 0.35001 turns
        constexpr uint16_t kEndpointBYPhase = 7307u; // 0.11150 turns
        constexpr uint16_t kLinePhaseStride = 8191u;
        constexpr uint16_t kLineSecondaryPhaseStride = 24593u;
        constexpr fl::s16x16 kLineRateSpread = s16x16FromFraction(3, 100); // 0.03
        constexpr fl::s16x16 kLineAmplitudeSpread = s16x16FromFraction(1, 20); // 0.05

        struct EndpointMotion {
            fl::s16x16 xAmplitude;
            fl::s16x16 yAmplitude;
            fl::s16x16 xRate;
            fl::s16x16 yRate;
            uint16_t xPhase;
            uint16_t yPhase;
        };

        constexpr EndpointMotion kEndpointA{
            s16x16FromFraction(23, 64), // 0.359375
            s16x16FromFraction(21, 64), // 0.328125
            s16x16FromMixed(1, 13, 100), // 1.13
            s16x16FromMixed(1, 71, 100), // 1.71
            kEndpointAXPhase,
            kEndpointAYPhase
        };

        constexpr EndpointMotion kEndpointB{
            s16x16FromFraction(3, 8), // 0.375
            s16x16FromFraction(11, 32), // 0.34375
            s16x16FromMixed(1, 89, 100), // 1.89
            s16x16FromMixed(1, 37, 100), // 1.37
            kEndpointBXPhase,
            kEndpointBYPhase
        };

        const BipolarRange<fl::s16x16> &profileSpeedRange() {
            static const BipolarRange range(-kMaxProfileSpeed, kMaxProfileSpeed);
            return range;
        }

        const MagnitudeRange<f16> &profileAmplitudeRange() {
            static const MagnitudeRange range(perMil(0), kMaxProfileAmplitude);
            return range;
        }

        const MagnitudeRange<fl::s16x16> &profileFrequencyRange() {
            static const MagnitudeRange range(kMinFrequency, kMaxProfileFrequency);
            return range;
        }

        const MagnitudeRange<fl::s16x16> &endpointSpeedRange() {
            static const MagnitudeRange range(s16x16FromFraction(0, 1), kMaxProfileSpeed);
            return range;
        }

        const MagnitudeRange<f16> &fadeRange() {
            static const MagnitudeRange range(kFadeBase, f16(raw(kFadeBase) + raw(kFadeSpan)));
            return range;
        }

        f16 intensityPerLine(uint8_t lineCount) {
            return f16(clampU16(std::max<uint32_t>(1u, kFullIntensity / static_cast<uint32_t>(lineCount))));
        }

        uint16_t phaseOffsetForLine(uint8_t lineIndex, uint16_t stride, uint16_t basePhase) {
            return static_cast<uint16_t>(basePhase + static_cast<uint16_t>(lineIndex * stride));
        }

        fl::s16x16 lineCenteredOffset(uint8_t lineIndex, uint8_t lineCount) {
            int32_t doubledIndex = static_cast<int32_t>(lineIndex) * 2;
            int32_t doubledCenter = static_cast<int32_t>(lineCount) - 1;
            return fl::s16x16::from_raw(
                static_cast<int32_t>(((doubledIndex - doubledCenter) * SF16_ONE) / 2)
            );
        }

    }

    struct FlurryPattern::State {
        uint8_t gridSize;
        uint8_t lineCount;
        Shape shape;
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
        Sf16Signal fadeSignal;
        fl::s16x16 xDrift{fl::s16x16::from_raw(0)};
        f16 xAmplitude{f16(0)};
        fl::s16x16 xFrequency{fl::s16x16::from_raw(0)};
        fl::s16x16 yDrift{fl::s16x16::from_raw(0)};
        f16 yAmplitude{f16(0)};
        fl::s16x16 yFrequency{fl::s16x16::from_raw(0)};
        fl::s16x16 endpointSpeed{fl::s16x16::from_raw(0)};
        f16 fadeFactor{f16(0)};
        fl::s16x16 timeQ16{fl::s16x16::from_raw(0)};
        TimeMillis lastElapsedMs{0u};
        bool hasLastElapsed{false};

        explicit State(
            uint8_t size,
            uint8_t lines,
            Shape patternShape,
            Sf16Signal xDrift,
            Sf16Signal yDrift,
            Sf16Signal amplitude,
            Sf16Signal frequency,
            Sf16Signal endpointSpeed,
            Sf16Signal fade
        ) : gridSize(size),
            lineCount(lines),
            shape(patternShape),
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
            fadeSignal(std::move(fade)) {
            std::fill_n(cells.get(), static_cast<size_t>(size) * size, uint16_t(0));
            std::fill_n(rowPass.get(), static_cast<size_t>(size) * size, uint16_t(0));
        }
    };

    struct FlurryPattern::FlurryFunctor {
        const State *state;

        PatternNormU16 operator()(UV uv) const {
            return sampleScalarGridClamped(state->cells.get(), state->gridSize, state->gridSize, uv);
        }
    };

    FlurryPattern::FlurryPattern(
        uint8_t gridSize,
        uint8_t lineCount,
        Shape shape,
        Sf16Signal xDrift,
        Sf16Signal yDrift,
        Sf16Signal amplitude,
        Sf16Signal frequency,
        Sf16Signal endpointSpeed,
        Sf16Signal fade
    ) : state(std::make_shared<State>(
        std::max<uint8_t>(kMinGridSize, std::min<uint8_t>(gridSize, kMaxGridSize)),
        std::max<uint8_t>(kMinLineCount, std::min<uint8_t>(lineCount, kMaxLineCount)),
        shape,
        std::move(xDrift),
        std::move(yDrift),
        std::move(amplitude),
        std::move(frequency),
        std::move(endpointSpeed),
        std::move(fade)
    )) {
    }

    void FlurryPattern::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void) progress;
        State &patternState = *state;

        if (!patternState.hasLastElapsed) {
            patternState.lastElapsedMs = elapsedMs;
            patternState.hasLastElapsed = true;
        } else {
            TimeMillis deltaMs = elapsedMs - patternState.lastElapsedMs;
            patternState.lastElapsedMs = elapsedMs;
            patternState.timeQ16 = patternState.timeQ16 +
                                   fl::s16x16::from_raw(raw(timeMillisToScalar(clampDeltaTime(deltaMs))));
        }

        patternState.xDrift = patternState.xDriftSignal.sample(profileSpeedRange(), elapsedMs);
        patternState.yDrift = patternState.yDriftSignal.sample(profileSpeedRange(), elapsedMs);
        patternState.xAmplitude = patternState.xAmplitudeSignal.sample(profileAmplitudeRange(), elapsedMs);
        patternState.yAmplitude = patternState.yAmplitudeSignal.sample(profileAmplitudeRange(), elapsedMs);
        patternState.xFrequency = patternState.xFrequencySignal.sample(profileFrequencyRange(), elapsedMs);
        patternState.yFrequency = patternState.yFrequencySignal.sample(profileFrequencyRange(), elapsedMs);
        patternState.endpointSpeed = patternState.endpointSpeedSignal.sample(endpointSpeedRange(), elapsedMs);
        patternState.fadeFactor = patternState.fadeSignal.sample(fadeRange(), elapsedMs);

        uint8_t gridSize = patternState.gridSize;
        fl::s16x16 xPhase = patternState.timeQ16 * patternState.xDrift;
        fl::s16x16 yPhase = patternState.timeQ16 * patternState.yDrift;
        for (uint8_t index = 0; index < gridSize; ++index) {
            fl::s16x16 profileCoord = fl::s16x16::from_raw(static_cast<int32_t>(index) * raw(kBaseNoiseFrequency));
            fl::s16x16 xProfileCoord = profileCoord * patternState.xFrequency;
            fl::s16x16 yProfileCoord = profileCoord * patternState.yFrequency;
            sf16 columnShift = scaleSf16(
                sampleProfileNoise(xProfileCoord + xPhase, patternState.xProfileNoiseSeed),
                patternState.xAmplitude
            );
            sf16 rowShift = scaleSf16(
                sampleProfileNoise(yProfileCoord + yPhase, patternState.yProfileNoiseSeed),
                patternState.yAmplitude
            );
            patternState.columnShiftProfile[gridSize - 1u - index] = columnShift;
            patternState.rowShiftProfile[index] = rowShift;
        }

        fl::s16x16 gridCenter = fl::s16x16::from_raw(static_cast<int32_t>(gridSize - 1u) * raw(kGridHalf));
        uint16_t *cells = patternState.cells.get();
        f16 lineIntensity = intensityPerLine(patternState.lineCount);
        for (uint8_t lineIndex = 0; lineIndex < patternState.lineCount; ++lineIndex) {
            fl::s16x16 lineOffset = lineCenteredOffset(lineIndex, patternState.lineCount);
            fl::s16x16 rateMultiplier = fl::s16x16::from_raw(SF16_ONE) + (lineOffset * kLineRateSpread);
            fl::s16x16 amplitudeMultiplier =
                fl::s16x16::from_raw(SF16_ONE) + (lineOffset * kLineAmplitudeSpread);

            fl::s16x16 endpointAXRate = patternState.endpointSpeed * (kEndpointA.xRate * rateMultiplier);
            fl::s16x16 endpointAYRate = patternState.endpointSpeed * (kEndpointA.yRate * rateMultiplier);
            fl::s16x16 endpointBXRate = patternState.endpointSpeed * (kEndpointB.xRate * rateMultiplier);
            fl::s16x16 endpointBYRate = patternState.endpointSpeed * (kEndpointB.yRate * rateMultiplier);

            f16 endpointAXPhase(static_cast<uint16_t>(
                raw(patternState.timeQ16 * endpointAXRate) +
                phaseOffsetForLine(lineIndex, kLinePhaseStride, kEndpointA.xPhase)
            ));
            f16 endpointAYPhase(static_cast<uint16_t>(
                raw(patternState.timeQ16 * endpointAYRate) +
                phaseOffsetForLine(lineIndex, kLineSecondaryPhaseStride, kEndpointA.yPhase)
            ));
            f16 endpointBXPhase(static_cast<uint16_t>(
                raw(patternState.timeQ16 * endpointBXRate) +
                phaseOffsetForLine(lineIndex, kLineSecondaryPhaseStride, kEndpointB.xPhase)
            ));
            f16 endpointBYPhase(static_cast<uint16_t>(
                raw(patternState.timeQ16 * endpointBYRate) +
                phaseOffsetForLine(lineIndex, kLinePhaseStride, kEndpointB.yPhase)
            ));

            fl::s16x16 endpointAX = gridCenter +
                                    mulS16x16(
                                        scaleByGridSize(gridSize, kEndpointA.xAmplitude * amplitudeMultiplier),
                                        angleSinF16(endpointAXPhase)
                                    );
            fl::s16x16 endpointAY = gridCenter +
                                    mulS16x16(
                                        scaleByGridSize(gridSize, kEndpointA.yAmplitude * amplitudeMultiplier),
                                        angleSinF16(endpointAYPhase)
                                    );
            fl::s16x16 endpointBX = gridCenter +
                                    mulS16x16(
                                        scaleByGridSize(gridSize, kEndpointB.xAmplitude * amplitudeMultiplier),
                                        angleSinF16(endpointBXPhase)
                                    );
            fl::s16x16 endpointBY = gridCenter +
                                    mulS16x16(
                                        scaleByGridSize(gridSize, kEndpointB.yAmplitude * amplitudeMultiplier),
                                        angleSinF16(endpointBYPhase)
                                    );

            if (patternState.shape == Shape::Ball) {
                fl::s16x16 ballX = (endpointAX + endpointBX) * kGridHalf;
                fl::s16x16 ballY = (endpointAY + endpointBY) * kGridHalf;
                fl::s16x16 ballRadius =
                    kBallDiscRadius * (fl::s16x16::from_raw(SF16_ONE) + (lineOffset * kBallRadiusSpread));
                fl::s16x16 ballSoftEdge =
                    kBallDiscSoftEdge * (fl::s16x16::from_raw(SF16_ONE) + (lineOffset * kBallRadiusSpread));
                drawSoftDisc(
                    cells,
                    gridSize,
                    ballX,
                    ballY,
                    ballRadius,
                    ballSoftEdge,
                    lineIntensity
                );
            } else {
                fl::s16x16 lineDeltaX = endpointBX - endpointAX;
                fl::s16x16 lineDeltaY = endpointBY - endpointAY;
                int32_t lineDeltaXRaw = raw(lineDeltaX);
                int32_t lineDeltaYRaw = raw(lineDeltaY);
                int32_t maxLineDelta = std::max(
                    lineDeltaXRaw < 0 ? -lineDeltaXRaw : lineDeltaXRaw,
                    lineDeltaYRaw < 0 ? -lineDeltaYRaw : lineDeltaYRaw
                );
                int32_t lineStepCount = std::max<int32_t>(1, (maxLineDelta * kLineSampleDensity) >> kQ16Shift);
                for (int32_t step = 0; step <= lineStepCount; ++step) {
                    uint32_t mixRaw = static_cast<uint32_t>(
                        (static_cast<uint64_t>(step) * kFullIntensity) / static_cast<uint32_t>(lineStepCount)
                    );
                    f16 mix(static_cast<uint16_t>(mixRaw));
                    drawSubpixelPoint(
                        cells,
                        gridSize,
                        lerpS16x16(endpointAX, endpointBX, mix),
                        lerpS16x16(endpointAY, endpointBY, mix),
                        lineIntensity
                    );
                }

                drawEndpointGlow(cells, gridSize, endpointAX, endpointAY, lineIntensity);
                drawEndpointGlow(cells, gridSize, endpointBX, endpointBY, lineIntensity);
            }
        }

        uint16_t *rowPass = patternState.rowPass.get();

        for (uint8_t y = 0; y < gridSize; ++y) {
            fl::s16x16 rowShift = mulS16x16(kRowShiftPixels, patternState.rowShiftProfile[y]);
            size_t rowBaseIndex = static_cast<size_t>(y) * gridSize;
            for (uint8_t x = 0; x < gridSize; ++x) {
                rowPass[rowBaseIndex + x] = sampleShiftedLaneWrapped(
                    cells,
                    rowBaseIndex,
                    1u,
                    gridSize,
                    x,
                    rowShift
                );
            }
        }

        for (uint8_t x = 0; x < gridSize; ++x) {
            fl::s16x16 columnShift = mulS16x16(kColShiftPixels, patternState.columnShiftProfile[x]);
            for (uint8_t y = 0; y < gridSize; ++y) {
                uint16_t advected = sampleShiftedLaneWrapped(
                    rowPass,
                    x,
                    gridSize,
                    gridSize,
                    y,
                    columnShift
                );
                cells[static_cast<size_t>(y) * gridSize + x] = scaleU16ByF16(advected, patternState.fadeFactor);
            }
        }
    }

    UVMap FlurryPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        (void) context;
        return FlurryFunctor{state.get()};
    }
}
