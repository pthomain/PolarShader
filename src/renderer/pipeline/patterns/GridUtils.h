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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_GRIDUTILS_H
#define POLAR_SHADER_PIPELINE_PATTERNS_GRIDUTILS_H

#include "renderer/pipeline/maths/Maths.h"
#include "renderer/pipeline/signals/SignalSamplers.h"
#include <algorithm>

namespace PolarShader {
    namespace grid {
        // ---- Constexpr fixed-point builders ----

        constexpr fl::s16x16 s16x16FromFraction(uint16_t numerator, uint16_t denominator) {
            return fl::s16x16::from_raw(raw(toF16(numerator, denominator)));
        }

        constexpr fl::s16x16 s16x16FromMixed(uint16_t whole, uint16_t numerator, uint16_t denominator) {
            return fl::s16x16::from_raw(static_cast<int32_t>(whole) * SF16_ONE + raw(toF16(numerator, denominator)));
        }

        // ---- Shared constants ----

        constexpr uint8_t kQ16Shift = 16;
        constexpr uint16_t kQ16FractionMask = F16_MAX;
        constexpr uint32_t kQ16FractionSpan = ANGLE_FULL_TURN_U32;
        constexpr uint16_t kFullIntensity = F16_MAX;
        constexpr uint8_t kMinGridSize = 4;
        constexpr uint8_t kMaxGridSize = 64;
        constexpr fl::s16x16 kGridHalf = s16x16FromFraction(1, 2);
        constexpr fl::s16x16 kRowShiftPixels = s16x16FromMixed(1, 4, 5); // 1.8
        constexpr fl::s16x16 kColShiftPixels = kRowShiftPixels;
        constexpr fl::s16x16 kBaseNoiseFrequency = s16x16FromFraction(23, 100); // 0.23
        constexpr fl::s16x16 kEndpointDiscRadius = s16x16FromFraction(60, 100); // 0.85
        constexpr fl::s16x16 kEndpointDiscSoftEdge = s16x16FromFraction(1, 2); // 0.5

        // ---- Grid drawing primitives ----

        // Blend a scalar cell toward full intensity by the given fractional weight.
        inline uint16_t blendTowardWhite(uint16_t current, f16 weight) {
            return lerpU16ByQ16(current, kFullIntensity, weight);
        }

        // Draw into one grid cell if the target coordinate is inside the simulation buffer.
        inline void blendPixel(uint16_t *cells, uint8_t gridSize, int32_t x, int32_t y, f16 weight) {
            if (x < 0 || y < 0 || x >= gridSize || y >= gridSize || raw(weight) == 0u) return;
            size_t index = static_cast<size_t>(y) * gridSize + static_cast<size_t>(x);
            cells[index] = blendTowardWhite(cells[index], weight);
        }

        // Rasterize a point at fractional cell coordinates with bilinear weights over the 4 neighbours.
        inline void drawSubpixelPoint(
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

        inline void drawSoftDisc(
            uint16_t *cells,
            uint8_t gridSize,
            fl::s16x16 xPos,
            fl::s16x16 yPos,
            fl::s16x16 radius,
            fl::s16x16 softEdge,
            f16 intensity
        ) {
            fl::s16x16 edgeRadius = radius + softEdge;
            int32_t minX = std::max<int32_t>(0, (raw(xPos - edgeRadius - fl::s16x16::from_raw(SF16_ONE)) >> kQ16Shift));
            int32_t maxX = std::min<int32_t>(
                gridSize - 1,
                (raw(xPos + edgeRadius + fl::s16x16::from_raw(SF16_ONE) - fl::s16x16::from_raw(1)) >> kQ16Shift)
            );
            int32_t minY = std::max<int32_t>(0, (raw(yPos - edgeRadius - fl::s16x16::from_raw(SF16_ONE)) >> kQ16Shift));
            int32_t maxY = std::min<int32_t>(
                gridSize - 1,
                (raw(yPos + edgeRadius + fl::s16x16::from_raw(SF16_ONE) - fl::s16x16::from_raw(1)) >> kQ16Shift)
            );

            for (int32_t py = minY; py <= maxY; ++py) {
                for (int32_t px = minX; px <= maxX; ++px) {
                    fl::s16x16 dx = fl::s16x16::from_raw(((px << kQ16Shift) + (SF16_ONE >> 1)) - raw(xPos));
                    fl::s16x16 dy = fl::s16x16::from_raw(((py << kQ16Shift) + (SF16_ONE >> 1)) - raw(yPos));
                    fl::s16x16 distance = fl::s16x16::sqrt(dx * dx + dy * dy);
                    int32_t weightRaw = raw(edgeRadius - distance);
                    if (weightRaw <= 0) continue;
                    if (weightRaw > SF16_ONE) weightRaw = SF16_ONE;
                    blendPixel(
                        cells,
                        gridSize,
                        px,
                        py,
                        f16(scaleU16ByF16(static_cast<uint16_t>(weightRaw), intensity))
                    );
                }
            }
        }

        // Draw a soft disc around an endpoint to seed the advection with a rounded source.
        inline void drawEndpointGlow(
            uint16_t *cells,
            uint8_t gridSize,
            fl::s16x16 xPos,
            fl::s16x16 yPos,
            f16 intensity
        ) {
            drawSoftDisc(cells, gridSize, xPos, yPos, kEndpointDiscRadius, kEndpointDiscSoftEdge, intensity);
        }

        // ---- Noise profile sampling ----

        // Sample a stable 1D noise profile by treating the coordinate as a single animated noise axis.
        inline sf16 sampleProfileNoise(fl::s16x16 coord, uint32_t seedOffset) {
            static const SampleSignal32 sampler = sampleNoise32();
            return sampler(static_cast<uint32_t>(raw(coord)) + seedOffset);
        }

        // ---- Lane advection ----

        // Read from a shifted row or column with wrapped bilinear sampling along that single axis.
        inline uint16_t sampleShiftedLaneWrapped(
            const uint16_t *sourceCells,
            size_t laneBaseIndex,
            size_t laneStride,
            uint8_t laneLength,
            uint8_t destinationIndex,
            fl::s16x16 laneShift
        ) {
            int32_t laneSpanRaw = static_cast<int32_t>(laneLength) << kQ16Shift;
            int32_t sampleCoordRaw = (static_cast<int32_t>(destinationIndex) << kQ16Shift) - raw(laneShift);
            sampleCoordRaw %= laneSpanRaw;
            if (sampleCoordRaw < 0) sampleCoordRaw += laneSpanRaw;
            fl::s16x16 sampleCoord = fl::s16x16::from_raw(sampleCoordRaw);
            uint8_t lowerIndex = static_cast<uint8_t>(raw(sampleCoord) >> kQ16Shift);
            uint8_t upperIndex = static_cast<uint8_t>((lowerIndex + 1u) % laneLength);
            f16 mix(static_cast<uint16_t>(raw(sampleCoord) & kQ16FractionMask));

            return lerpU16ByQ16(
                sourceCells[laneBaseIndex + static_cast<size_t>(lowerIndex) * laneStride],
                sourceCells[laneBaseIndex + static_cast<size_t>(upperIndex) * laneStride],
                mix
            );
        }

        // ---- Grid scale helper ----

        inline fl::s16x16 scaleByGridSize(uint8_t gridSize, fl::s16x16 scale) {
            return fl::s16x16::from_raw(static_cast<int32_t>(gridSize) * raw(scale));
        }

        // ---- Half-life fade ----

        /// Framerate-independent fade: pow(0.5, dt / halfLife) as f16.
        /// Uses a 17-entry constexpr LUT for pow(0.5, x) where x in [0, 1].
        /// dt and halfLifeMs are in milliseconds.
        inline f16 halfLifeFade(TimeMillis dtMs, uint16_t halfLifeMs) {
            if (halfLifeMs == 0u) return f16(0);
            if (dtMs == 0u) return f16(F16_MAX);

            // Compute ratio = dt / halfLife in Q4.12 fixed-point.
            // Max dt is 200ms (clamped), max ratio is ~2.0 in Q4.12 = 8192.
            uint32_t ratioQ12 = (static_cast<uint32_t>(dtMs) << 12) / static_cast<uint32_t>(halfLifeMs);

            // pow(0.5, i/16) for i in [0..16], as f16 values.
            // LUT[0] = pow(0.5, 0) = 1.0 = 65535
            // LUT[16] = pow(0.5, 1) = 0.5 = 32768
            static constexpr uint16_t kHalfLifeLUT[17] = {
                65535u, 62757u, 60096u, 57548u, 55108u, 52772u, 50534u, 48392u,
                46340u, 44376u, 42494u, 40693u, 38967u, 37315u, 35733u, 34218u,
                32768u
            };
            // Split ratio into integer + fractional parts.
            // Integer part n: result >>= n (each integer step halves).
            // Fractional part f (0..1): index into LUT with linear interpolation.

            // Extract integer and fractional parts from Q4.12.
            // ratio = n + f where n is integer, f in [0, 1).
            // In Q4.12: integer part = ratioQ12 >> 12, fractional = ratioQ12 & 0xFFF.
            uint32_t intPart = ratioQ12 >> 12;
            uint32_t fracQ12 = ratioQ12 & 0xFFFu;

            // Map fractional part to LUT index [0..16] in Q4.12 -> Q4.8 for 16 steps.
            // lutIndex = fracQ12 * 16 / 4096 = fracQ12 >> 8
            uint32_t lutScaled = fracQ12 << 4; // fracQ12 * 16, now Q4.16
            uint32_t lutIndex = lutScaled >> 12;
            if (lutIndex > 15u) lutIndex = 15u;
            uint32_t lutFrac = (lutScaled >> 4) & 0xFFu; // 8-bit interpolation fraction
            uint16_t lutFracF16 = static_cast<uint16_t>(lutFrac << 8); // scale to Q0.16

            uint16_t base = kHalfLifeLUT[lutIndex];
            uint16_t next = kHalfLifeLUT[lutIndex + 1u];
            uint16_t interpolated = lerpU16ByQ16(base, next, f16(lutFracF16));

            // Apply integer halving: result >>= intPart.
            // Cap to prevent zero for very large ratios.
            if (intPart >= 16u) return f16(0);
            uint32_t result = static_cast<uint32_t>(interpolated) >> intPart;

            return f16(static_cast<uint16_t>(result));
        }

        // ---- Lissajous emitter ----

        constexpr int32_t kLineSampleDensity = 3;

        struct EndpointMotion {
            fl::s16x16 xAmplitude;
            fl::s16x16 yAmplitude;
            fl::s16x16 xRate;
            fl::s16x16 yRate;
            uint16_t xPhase;
            uint16_t yPhase;
        };

        constexpr EndpointMotion kLissajousEndpointA{
            s16x16FromFraction(23, 64),
            s16x16FromFraction(21, 64),
            s16x16FromMixed(1, 13, 100),
            s16x16FromMixed(1, 71, 100),
            2086u, 13557u
        };

        constexpr EndpointMotion kLissajousEndpointB{
            s16x16FromFraction(3, 8),
            s16x16FromFraction(11, 32),
            s16x16FromMixed(1, 89, 100),
            s16x16FromMixed(1, 37, 100),
            22938u, 7307u
        };

        /// Emit a Lissajous line + endpoint glows onto a scalar grid.
        /// speed is the emitter speed (fl::s16x16), timeQ16 is accumulated Q16.16 time.
        inline void emitLissajousLine(
            uint16_t *cells,
            uint8_t gridSize,
            fl::s16x16 gridCenter,
            fl::s16x16 timeQ16,
            fl::s16x16 speed
        ) {
            f16 aXPh(static_cast<uint16_t>(raw(timeQ16 * (speed * kLissajousEndpointA.xRate)) + kLissajousEndpointA.xPhase));
            f16 aYPh(static_cast<uint16_t>(raw(timeQ16 * (speed * kLissajousEndpointA.yRate)) + kLissajousEndpointA.yPhase));
            f16 bXPh(static_cast<uint16_t>(raw(timeQ16 * (speed * kLissajousEndpointB.xRate)) + kLissajousEndpointB.xPhase));
            f16 bYPh(static_cast<uint16_t>(raw(timeQ16 * (speed * kLissajousEndpointB.yRate)) + kLissajousEndpointB.yPhase));

            fl::s16x16 ax = gridCenter + mulS16x16(scaleByGridSize(gridSize, kLissajousEndpointA.xAmplitude), angleSinF16(aXPh));
            fl::s16x16 ay = gridCenter + mulS16x16(scaleByGridSize(gridSize, kLissajousEndpointA.yAmplitude), angleSinF16(aYPh));
            fl::s16x16 bx = gridCenter + mulS16x16(scaleByGridSize(gridSize, kLissajousEndpointB.xAmplitude), angleSinF16(bXPh));
            fl::s16x16 by = gridCenter + mulS16x16(scaleByGridSize(gridSize, kLissajousEndpointB.yAmplitude), angleSinF16(bYPh));

            fl::s16x16 ldx = bx - ax;
            fl::s16x16 ldy = by - ay;
            int32_t maxDelta = std::max(
                raw(ldx) < 0 ? -raw(ldx) : raw(ldx),
                raw(ldy) < 0 ? -raw(ldy) : raw(ldy)
            );
            int32_t steps = std::max<int32_t>(1, (maxDelta * kLineSampleDensity) >> kQ16Shift);
            for (int32_t step = 0; step <= steps; ++step) {
                uint32_t mixRaw = (static_cast<uint64_t>(step) * kFullIntensity) / static_cast<uint32_t>(steps);
                f16 mix(static_cast<uint16_t>(mixRaw));
                drawSubpixelPoint(cells, gridSize, lerpS16x16(ax, bx, mix), lerpS16x16(ay, by, mix));
            }
            drawEndpointGlow(cells, gridSize, ax, ay, f16(kFullIntensity));
            drawEndpointGlow(cells, gridSize, bx, by, f16(kFullIntensity));
        }

        // ---- 2D backward advection ----

        /// Bilinear sample from a scalar grid at fractional Q16.16 coordinates with wrapping.
        inline uint16_t sampleGridBilinearWrapped(
            const uint16_t *source,
            uint8_t gridSize,
            int32_t xRaw,   // Q16.16 grid-cell coordinate
            int32_t yRaw
        ) {
            int32_t spanRaw = static_cast<int32_t>(gridSize) << kQ16Shift;

            // Wrap to [0, spanRaw). Subtract-loop avoids expensive modulo on M0.
            while (xRaw < 0) xRaw += spanRaw;
            while (xRaw >= spanRaw) xRaw -= spanRaw;
            while (yRaw < 0) yRaw += spanRaw;
            while (yRaw >= spanRaw) yRaw -= spanRaw;

            uint8_t x0 = static_cast<uint8_t>(xRaw >> kQ16Shift);
            uint8_t y0 = static_cast<uint8_t>(yRaw >> kQ16Shift);
            uint8_t x1 = (x0 + 1u < gridSize) ? (x0 + 1u) : 0u;
            uint8_t y1 = (y0 + 1u < gridSize) ? (y0 + 1u) : 0u;
            uint16_t fx = static_cast<uint16_t>(xRaw & kQ16FractionMask);
            uint16_t fy = static_cast<uint16_t>(yRaw & kQ16FractionMask);

            uint16_t v00 = source[static_cast<size_t>(y0) * gridSize + x0];
            uint16_t v10 = source[static_cast<size_t>(y0) * gridSize + x1];
            uint16_t v01 = source[static_cast<size_t>(y1) * gridSize + x0];
            uint16_t v11 = source[static_cast<size_t>(y1) * gridSize + x1];

            uint16_t top = lerpU16ByQ16(v00, v10, f16(fx));
            uint16_t bot = lerpU16ByQ16(v01, v11, f16(fx));
            return lerpU16ByQ16(top, bot, f16(fy));
        }

        /// Per-pixel 2D backward advection with per-pixel vector callback.
        /// vectorFn returns displacement in Q16.16 grid-cell units.
        using TransportVectorFn = v32(*)(
            uint8_t x, uint8_t y, uint8_t gridSize, const void *params
        );

        inline void advectGrid2DBackward(
            const uint16_t *source,
            uint16_t *dest,
            uint8_t gridSize,
            TransportVectorFn vectorFn,
            const void *params,
            f16 fadeFactor
        ) {
            for (uint8_t y = 0; y < gridSize; ++y) {
                int32_t yCenter = (static_cast<int32_t>(y) << kQ16Shift) + (SF16_ONE >> 1);
                for (uint8_t x = 0; x < gridSize; ++x) {
                    int32_t xCenter = (static_cast<int32_t>(x) << kQ16Shift) + (SF16_ONE >> 1);
                    v32 disp = vectorFn(x, y, gridSize, params);
                    int32_t srcX = xCenter - disp.x;
                    int32_t srcY = yCenter - disp.y;
                    uint16_t sampled = sampleGridBilinearWrapped(source, gridSize, srcX, srcY);
                    dest[static_cast<size_t>(y) * gridSize + x] = scaleU16ByF16(sampled, fadeFactor);
                }
            }
        }

    } // namespace grid
} // namespace PolarShader

#endif // POLAR_SHADER_PIPELINE_PATTERNS_GRIDUTILS_H
