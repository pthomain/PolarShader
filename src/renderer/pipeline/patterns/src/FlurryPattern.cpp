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
#include "renderer/pipeline/maths/Maths.h"
#include "renderer/pipeline/signals/SignalSamplers.h"
#include "renderer/pipeline/signals/ranges/BipolarRange.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"
#include <algorithm>

namespace PolarShader {
    namespace {
        constexpr uint8_t kMinGridSize = 4;
        constexpr uint8_t kMaxGridSize = 64;
        constexpr uint8_t kQ16Shift = 16;
        constexpr uint16_t kQ16FractionMask = F16_MAX;
        constexpr uint32_t kQ16FractionSpan = ANGLE_FULL_TURN_U32;
        constexpr fl::s16x16 kGridHalf = fl::s16x16::from_raw(SF16_ONE >> 1);
        constexpr fl::s16x16 kMinFrequency = fl::s16x16::from_raw(1024); // 1/64
        constexpr fl::s16x16 kRowShiftPixels = fl::s16x16::from_raw(117965); // 1.8
        constexpr fl::s16x16 kColShiftPixels = fl::s16x16::from_raw(117965); // 1.8
        constexpr fl::s16x16 kBaseNoiseFrequency = fl::s16x16::from_raw(15073); // 0.23
        constexpr f16 kFadeBase = f16(58982); // 0.90
        constexpr f16 kFadeSpan = f16(6553); // ~0.09999
        constexpr uint16_t kFullIntensity = F16_MAX;
        constexpr uint32_t kXProfileNoiseSeed = 0x13579BDFu;
        constexpr uint32_t kYProfileNoiseSeed = 0x2468ACE1u;

        struct EndpointMotion {
            fl::s16x16 xAmplitude;
            fl::s16x16 yAmplitude;
            fl::s16x16 xRate;
            fl::s16x16 yRate;
            uint16_t xPhase;
            uint16_t yPhase;
        };

        constexpr EndpointMotion kEndpointA{
            fl::s16x16::from_raw(23552),
            fl::s16x16::from_raw(21504),
            fl::s16x16::from_raw(74056),
            fl::s16x16::from_raw(112066),
            2086u,
            13557u
        };

        constexpr EndpointMotion kEndpointB{
            fl::s16x16::from_raw(24576),
            fl::s16x16::from_raw(22528),
            fl::s16x16::from_raw(123863),
            fl::s16x16::from_raw(89784),
            22938u,
            7307u
        };

        constexpr fl::s16x16 kMaxProfileSpeed = fl::s16x16::from_raw(SF16_ONE * 2);
        constexpr fl::s16x16 kMaxProfileFrequency = fl::s16x16::from_raw(SF16_ONE * 4);
        constexpr fl::s16x16 kEndpointGlowOffset = fl::s16x16::from_raw(SF16_ONE >> 2);
        constexpr uint16_t kEndpointGlowIntensity = 24576u;
        constexpr int32_t kLineSampleDensity = 3;

        const BipolarRange<fl::s16x16> &profileSpeedRange() {
            static const BipolarRange range(-kMaxProfileSpeed, kMaxProfileSpeed);
            return range;
        }

        const MagnitudeRange<f16> &profileAmplitudeRange() {
            static const MagnitudeRange range(f16(0), f16(F16_MAX));
            return range;
        }

        const MagnitudeRange<fl::s16x16> &profileFrequencyRange() {
            static const MagnitudeRange range(kMinFrequency, kMaxProfileFrequency);
            return range;
        }

        const MagnitudeRange<fl::s16x16> &endpointSpeedRange() {
            static const MagnitudeRange range(fl::s16x16::from_raw(0), kMaxProfileSpeed);
            return range;
        }

        const MagnitudeRange<f16> &fadeRange() {
            static const MagnitudeRange range(kFadeBase, f16(raw(kFadeBase) + raw(kFadeSpan)));
            return range;
        }

        fl::s16x16 scaleByGridSize(uint8_t gridSize, fl::s16x16 scale) {
            return fl::s16x16::from_raw(static_cast<int32_t>(gridSize) * raw(scale));
        }

        // Blend a scalar cell toward full intensity by the given fractional weight.
        uint16_t blendTowardWhite(uint16_t current, f16 weight) {
            return lerpU16ByQ16(current, kFullIntensity, weight);
        }

        // Draw into one grid cell if the target coordinate is inside the simulation buffer.
        void blendPixel(uint16_t *cells, uint8_t gridSize, int32_t x, int32_t y, f16 weight) {
            if (x < 0 || y < 0 || x >= gridSize || y >= gridSize || raw(weight) == 0u) return;
            size_t index = static_cast<size_t>(y) * gridSize + static_cast<size_t>(x);
            cells[index] = blendTowardWhite(cells[index], weight);
        }

        // Rasterize a point at fractional cell coordinates with bilinear weights over the 4 neighbours.
        void drawSubpixelPoint(
            uint16_t *cells,
            uint8_t gridSize,
            fl::s16x16 xPos,
            fl::s16x16 yPos,
            f16 intensity = f16(kFullIntensity)
        ) {
            int32_t baseX = raw(xPos) >> kQ16Shift;
            int32_t baseY = raw(yPos) >> kQ16Shift;
            uint32_t fracX = static_cast<uint32_t>(raw(xPos)) & kQ16FractionMask;
            uint32_t fracY = static_cast<uint32_t>(raw(yPos)) & kQ16FractionMask;
            uint32_t invFracX = kQ16FractionSpan - fracX;
            uint32_t invFracY = kQ16FractionSpan - fracY;

            f16 topLeftWeight(clampU16(
                static_cast<uint32_t>((static_cast<uint64_t>(invFracX) * invFracY * raw(intensity)) >> (kQ16Shift * 2))
            ));
            f16 topRightWeight(clampU16(
                static_cast<uint32_t>((static_cast<uint64_t>(fracX) * invFracY * raw(intensity)) >> (kQ16Shift * 2))
            ));
            f16 bottomLeftWeight(clampU16(
                static_cast<uint32_t>((static_cast<uint64_t>(invFracX) * fracY * raw(intensity)) >> (kQ16Shift * 2))
            ));
            f16 bottomRightWeight(clampU16(
                static_cast<uint32_t>((static_cast<uint64_t>(fracX) * fracY * raw(intensity)) >> (kQ16Shift * 2))
            ));

            blendPixel(cells, gridSize, baseX, baseY, topLeftWeight);
            blendPixel(cells, gridSize, baseX + 1, baseY, topRightWeight);
            blendPixel(cells, gridSize, baseX, baseY + 1, bottomLeftWeight);
            blendPixel(cells, gridSize, baseX + 1, baseY + 1, bottomRightWeight);
        }

        // Draw a bright endpoint marker with a small cross-shaped glow around the center.
        void drawEndpointGlow(uint16_t *cells, uint8_t gridSize, fl::s16x16 xPos, fl::s16x16 yPos) {
            drawSubpixelPoint(cells, gridSize, xPos, yPos, f16(kFullIntensity));
            drawSubpixelPoint(cells, gridSize, xPos + kEndpointGlowOffset, yPos, f16(kEndpointGlowIntensity));
            drawSubpixelPoint(cells, gridSize, xPos - kEndpointGlowOffset, yPos, f16(kEndpointGlowIntensity));
            drawSubpixelPoint(cells, gridSize, xPos, yPos + kEndpointGlowOffset, f16(kEndpointGlowIntensity));
            drawSubpixelPoint(cells, gridSize, xPos, yPos - kEndpointGlowOffset, f16(kEndpointGlowIntensity));
        }

        // Sample a stable 1D noise profile by treating the coordinate as a single animated noise axis.
        sf16 sampleProfileNoise(fl::s16x16 coord, uint32_t seedOffset) {
            static const SampleSignal32 sampler = sampleNoise32();
            return sampler(static_cast<uint32_t>(raw(coord)) + seedOffset);
        }

        // Read from a shifted row or column with clamped bilinear sampling along that single axis.
        uint16_t sampleShiftedLaneClamped(
            const uint16_t *sourceCells,
            size_t laneBaseIndex,
            size_t laneStride,
            uint8_t laneLength,
            uint8_t destinationIndex,
            fl::s16x16 laneShift,
            fl::s16x16 maxLaneCoord
        ) {
            fl::s16x16 sampleCoord = clampS16x16(
                fl::s16x16::from_raw(static_cast<int32_t>(destinationIndex) << kQ16Shift) - laneShift,
                fl::s16x16::from_raw(0),
                maxLaneCoord
            );
            uint8_t lowerIndex = static_cast<uint8_t>(raw(sampleCoord) >> kQ16Shift);
            uint8_t upperIndex = static_cast<uint8_t>(
                (lowerIndex + 1u < laneLength) ? (lowerIndex + 1u) : (laneLength - 1u)
            );
            f16 mix(static_cast<uint16_t>(raw(sampleCoord) & kQ16FractionMask));

            return lerpU16ByQ16(
                sourceCells[laneBaseIndex + static_cast<size_t>(lowerIndex) * laneStride],
                sourceCells[laneBaseIndex + static_cast<size_t>(upperIndex) * laneStride],
                mix
            );
        }
    }

    struct FlurryPattern::State {
        uint8_t gridSize;
        std::unique_ptr<uint16_t[]> cells;
        std::unique_ptr<uint16_t[]> rowPass;
        std::unique_ptr<sf16[]> columnShiftProfile;
        std::unique_ptr<sf16[]> rowShiftProfile;
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
            Sf16Signal xDrift,
            Sf16Signal xAmplitude,
            Sf16Signal xFrequency,
            Sf16Signal yDrift,
            Sf16Signal yAmplitude,
            Sf16Signal yFrequency,
            Sf16Signal endpointSpeed,
            Sf16Signal fade
        ) : gridSize(size),
            cells(std::make_unique<uint16_t[]>(static_cast<size_t>(size) * size)),
            rowPass(std::make_unique<uint16_t[]>(static_cast<size_t>(size) * size)),
            columnShiftProfile(std::make_unique<sf16[]>(size)),
            rowShiftProfile(std::make_unique<sf16[]>(size)),
            xDriftSignal(std::move(xDrift)),
            xAmplitudeSignal(std::move(xAmplitude)),
            xFrequencySignal(std::move(xFrequency)),
            yDriftSignal(std::move(yDrift)),
            yAmplitudeSignal(std::move(yAmplitude)),
            yFrequencySignal(std::move(yFrequency)),
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
        Sf16Signal xDrift,
        Sf16Signal xAmplitude,
        Sf16Signal xFrequency,
        Sf16Signal yDrift,
        Sf16Signal yAmplitude,
        Sf16Signal yFrequency,
        Sf16Signal endpointSpeed,
        Sf16Signal fade
    ) : state(std::make_shared<State>(
        std::max<uint8_t>(kMinGridSize, std::min<uint8_t>(gridSize, kMaxGridSize)),
        std::move(xDrift),
        std::move(xAmplitude),
        std::move(xFrequency),
        std::move(yDrift),
        std::move(yAmplitude),
        std::move(yFrequency),
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
                sampleProfileNoise(xProfileCoord + xPhase, kXProfileNoiseSeed),
                patternState.xAmplitude
            );
            sf16 rowShift = scaleSf16(
                sampleProfileNoise(yProfileCoord + yPhase, kYProfileNoiseSeed),
                patternState.yAmplitude
            );
            patternState.columnShiftProfile[gridSize - 1u - index] = columnShift;
            patternState.rowShiftProfile[index] = rowShift;
        }

        fl::s16x16 endpointAXRate = patternState.endpointSpeed * kEndpointA.xRate;
        fl::s16x16 endpointAYRate = patternState.endpointSpeed * kEndpointA.yRate;
        fl::s16x16 endpointBXRate = patternState.endpointSpeed * kEndpointB.xRate;
        fl::s16x16 endpointBYRate = patternState.endpointSpeed * kEndpointB.yRate;

        f16 endpointAXPhase(
            static_cast<uint16_t>(raw(patternState.timeQ16 * endpointAXRate) + kEndpointA.xPhase));
        f16 endpointAYPhase(
            static_cast<uint16_t>(raw(patternState.timeQ16 * endpointAYRate) + kEndpointA.yPhase));
        f16 endpointBXPhase(
            static_cast<uint16_t>(raw(patternState.timeQ16 * endpointBXRate) + kEndpointB.xPhase));
        f16 endpointBYPhase(
            static_cast<uint16_t>(raw(patternState.timeQ16 * endpointBYRate) + kEndpointB.yPhase));

        fl::s16x16 gridCenter = fl::s16x16::from_raw(static_cast<int32_t>(gridSize - 1u) * raw(kGridHalf));
        fl::s16x16 endpointAX = gridCenter +
                                mulS16x16(scaleByGridSize(gridSize, kEndpointA.xAmplitude),
                                          angleSinF16(endpointAXPhase));
        fl::s16x16 endpointAY = gridCenter +
                                mulS16x16(scaleByGridSize(gridSize, kEndpointA.yAmplitude),
                                          angleSinF16(endpointAYPhase));
        fl::s16x16 endpointBX = gridCenter +
                                mulS16x16(scaleByGridSize(gridSize, kEndpointB.xAmplitude),
                                          angleSinF16(endpointBXPhase));
        fl::s16x16 endpointBY = gridCenter +
                                mulS16x16(scaleByGridSize(gridSize, kEndpointB.yAmplitude),
                                          angleSinF16(endpointBYPhase));

        uint16_t *cells = patternState.cells.get();
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
                lerpS16x16(endpointAY, endpointBY, mix)
            );
        }

        drawEndpointGlow(cells, gridSize, endpointAX, endpointAY);
        drawEndpointGlow(cells, gridSize, endpointBX, endpointBY);

        uint16_t *rowPass = patternState.rowPass.get();
        fl::s16x16 maxGridCoord = fl::s16x16::from_raw(static_cast<int32_t>(gridSize - 1u) << kQ16Shift);

        for (uint8_t y = 0; y < gridSize; ++y) {
            fl::s16x16 rowShift = mulS16x16(kRowShiftPixels, patternState.rowShiftProfile[y]);
            size_t rowBaseIndex = static_cast<size_t>(y) * gridSize;
            for (uint8_t x = 0; x < gridSize; ++x) {
                rowPass[rowBaseIndex + x] = sampleShiftedLaneClamped(
                    cells,
                    rowBaseIndex,
                    1u,
                    gridSize,
                    x,
                    rowShift,
                    maxGridCoord
                );
            }
        }

        for (uint8_t x = 0; x < gridSize; ++x) {
            fl::s16x16 columnShift = mulS16x16(kColShiftPixels, patternState.columnShiftProfile[x]);
            for (uint8_t y = 0; y < gridSize; ++y) {
                uint16_t advected = sampleShiftedLaneClamped(
                    rowPass,
                    x,
                    gridSize,
                    gridSize,
                    y,
                    columnShift,
                    maxGridCoord
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
