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

#include "renderer/pipeline/patterns/TransportPattern.h"
#include "renderer/pipeline/patterns/GridUtils.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"
#include <algorithm>

namespace PolarShader {
    using namespace grid;

    namespace {
        constexpr uint16_t kHalfLifeMinMs = 100u;
        constexpr uint16_t kHalfLifeMaxMs = 5000u;

        constexpr fl::s16x16 kMaxRadialSpeed = s16x16FromFraction(3, 1); // 3.0 cells/frame
        constexpr fl::s16x16 kMaxAngularSpeed = s16x16FromFraction(1, 8); // 1/8 turn/frame

        // Shockwave constants.
        constexpr fl::s16x16 kShockwaveWidthCells = s16x16FromFraction(2, 1); // 2.0 cells
        constexpr fl::s16x16 kShockwaveAmplitude = s16x16FromFraction(2, 1);

        // Fractal spiral constants.
        constexpr uint8_t kFractalSteps = 4;
        constexpr fl::s16x16 kFractalRadiusStep = s16x16FromFraction(1, 16);
        constexpr fl::s16x16 kFractalAngleScale = s16x16FromFraction(1, 4);

        // Attractor constants.
        constexpr uint8_t kAttractorCount = 3;
        constexpr fl::s16x16 kAttractorSwirlRatio = s16x16FromFraction(1, 2); // tangential/radial
        constexpr int32_t kAttractorMinDist2 = SF16_ONE; // prevent singularity

        const MagnitudeRange<fl::s16x16> &radialSpeedRange() {
            static const MagnitudeRange range(fl::s16x16::from_raw(0), kMaxRadialSpeed);
            return range;
        }

        const MagnitudeRange<fl::s16x16> &angularSpeedRange() {
            static const MagnitudeRange range(fl::s16x16::from_raw(0), kMaxAngularSpeed);
            return range;
        }

        const MagnitudeRange<fl::s16x16> &emitterSpeedRange() {
            static const MagnitudeRange range(fl::s16x16::from_raw(0), s16x16FromFraction(1, 4));
            return range;
        }

        // ---- Polar coordinate helpers ----

        struct PolarCoord {
            int32_t radiusRaw; // Q16.16, distance from center in grid cells
            f16 angle;         // f16 turns [0, 65535] = [0, 2pi)
            int32_t cosA;      // sf16 raw of cos(angle)
            int32_t sinA;      // sf16 raw of sin(angle)
        };

        // Convert grid cell (x, y) to polar relative to grid center.
        // Returns radius in Q16.16 grid-cell units, angle as f16.
        PolarCoord gridToPolar(uint8_t x, uint8_t y, uint8_t gridSize) {
            int32_t halfRaw = (static_cast<int32_t>(gridSize - 1u) << kQ16Shift) >> 1;
            int32_t dxRaw = (static_cast<int32_t>(x) << kQ16Shift) + (SF16_ONE >> 1) - halfRaw;
            int32_t dyRaw = (static_cast<int32_t>(y) << kQ16Shift) + (SF16_ONE >> 1) - halfRaw;

            // Radius = sqrt(dx² + dy²).
            uint64_t r2 = static_cast<uint64_t>(
                static_cast<int64_t>(dxRaw) * dxRaw + static_cast<int64_t>(dyRaw) * dyRaw
            );
            int32_t radiusRaw = static_cast<int32_t>(sqrtU64Raw(r2));

            // Angle via atan2 approximation.
            // Scale dx, dy to int16 range for angleAtan2TurnsApprox.
            int16_t dxScaled, dyScaled;
            if (radiusRaw > 0) {
                int32_t shift = 0;
                int32_t maxComp = std::max(
                    dxRaw < 0 ? -dxRaw : dxRaw,
                    dyRaw < 0 ? -dyRaw : dyRaw
                );
                // Normalise to fit int16 range.
                while (maxComp > 0x7FFF && shift < 16) {
                    maxComp >>= 1;
                    shift++;
                }
                dxScaled = static_cast<int16_t>(dxRaw >> shift);
                dyScaled = static_cast<int16_t>(dyRaw >> shift);
            } else {
                dxScaled = 0;
                dyScaled = 0;
            }

            f16 angle = angleAtan2TurnsApprox(dyScaled, dxScaled);

            int32_t cosA = raw(angleCosF16(angle));
            int32_t sinA = raw(angleSinF16(angle));

            return PolarCoord{radiusRaw, angle, cosA, sinA};
        }

        // Convert polar displacement (dr, dTheta) to cartesian (dx, dy) in Q16.16.
        // dr is Q16.16 grid-cell units, dTheta is Q16.16 radians-like (but we use turns).
        // dTheta is in turns * SF16_ONE scale.
        v32 polarToCartesian(const PolarCoord &polar, int32_t drRaw, int32_t dThetaRaw) {
            // dx = dr * cos(a) - r * dTheta * sin(a)
            // dy = dr * sin(a) + r * dTheta * cos(a)
            int32_t drCos = static_cast<int32_t>((static_cast<int64_t>(drRaw) * polar.cosA) >> 16);
            int32_t drSin = static_cast<int32_t>((static_cast<int64_t>(drRaw) * polar.sinA) >> 16);
            int32_t rDTheta = static_cast<int32_t>((static_cast<int64_t>(polar.radiusRaw) * dThetaRaw) >> 16);
            int32_t rDThetaSin = static_cast<int32_t>((static_cast<int64_t>(rDTheta) * polar.sinA) >> 16);
            int32_t rDThetaCos = static_cast<int32_t>((static_cast<int64_t>(rDTheta) * polar.cosA) >> 16);

            return v32{drCos - rDThetaSin, drSin + rDThetaCos};
        }

        // ---- Transport mode parameters passed to vector callbacks ----

        struct TransportParams {
            TransportPattern::TransportMode mode;
            uint8_t gridSize;
            int32_t radialSpeedRaw;    // Q16.16 grid-cell displacement per frame
            int32_t angularSpeedRaw;   // Q16.16 angular displacement (turns * SF16_ONE)
            int32_t timeRaw;           // Q16.16 accumulated time
            // Shockwave state.
            int32_t shockwaveRadiusRaw;
            // Attractor positions (Q16.16 grid-cell coords).
            int32_t attractorX[kAttractorCount];
            int32_t attractorY[kAttractorCount];
        };

        // ---- Vector field functions per mode ----

        v32 transportVector(uint8_t x, uint8_t y, uint8_t gridSize, const void *params) {
            const auto &p = *static_cast<const TransportParams *>(params);
            PolarCoord polar = gridToPolar(x, y, gridSize);

            switch (p.mode) {
                case TransportPattern::TransportMode::SpiralInward: {
                    return polarToCartesian(polar, -p.radialSpeedRaw, p.angularSpeedRaw);
                }

                case TransportPattern::TransportMode::SpiralOutward: {
                    return polarToCartesian(polar, p.radialSpeedRaw, p.angularSpeedRaw);
                }

                case TransportPattern::TransportMode::RadialSink: {
                    return polarToCartesian(polar, -p.radialSpeedRaw, 0);
                }

                case TransportPattern::TransportMode::RadialSource: {
                    return polarToCartesian(polar, p.radialSpeedRaw, 0);
                }

                case TransportPattern::TransportMode::RotatingSwirl: {
                    return polarToCartesian(polar, 0, p.angularSpeedRaw);
                }

                case TransportPattern::TransportMode::PolarPulse: {
                    // Radial pulsation: dr = sin(time * angularSpeed) * amplitude.
                    f16 pulsePhase(static_cast<uint16_t>(
                        (static_cast<int64_t>(p.timeRaw) * p.angularSpeedRaw) >> 16
                    ));
                    int32_t pulseSin = raw(angleSinF16(pulsePhase));
                    int32_t dr = static_cast<int32_t>((static_cast<int64_t>(pulseSin) * p.radialSpeedRaw) >> 16);
                    return polarToCartesian(polar, dr, 0);
                }

                case TransportPattern::TransportMode::Shockwave: {
                    // Gaussian-like displacement peak at wavefront radius.
                    int32_t delta = polar.radiusRaw - p.shockwaveRadiusRaw;
                    int32_t widthRaw = raw(kShockwaveWidthCells);
                    if (widthRaw == 0) widthRaw = 1;
                    // Normalise delta to [-1, 1] over width.
                    int32_t xNorm = static_cast<int32_t>((static_cast<int64_t>(delta) << 16) / widthRaw);
                    int32_t strength;
                    if (xNorm < -(1 << 16) || xNorm > (1 << 16)) {
                        strength = 0;
                    } else {
                        // (1 - x²)² approximation.
                        int32_t x2 = static_cast<int32_t>((static_cast<int64_t>(xNorm) * xNorm) >> 16);
                        int32_t oneMinusX2 = SF16_ONE - x2;
                        if (oneMinusX2 < 0) oneMinusX2 = 0;
                        strength = static_cast<int32_t>((static_cast<int64_t>(oneMinusX2) * oneMinusX2) >> 16);
                    }
                    int32_t dr = static_cast<int32_t>((static_cast<int64_t>(strength) * raw(kShockwaveAmplitude)) >> 16);
                    return polarToCartesian(polar, dr, 0);
                }

                case TransportPattern::TransportMode::FractalSpiral: {
                    // Iterative: radius increments while angle steps by sin(radius).
                    int32_t totalDr = 0;
                    int32_t totalDTheta = 0;
                    int32_t rCurrent = polar.radiusRaw;
                    for (uint8_t step = 0; step < kFractalSteps; ++step) {
                        // Angle step depends on current radius.
                        int32_t rScaled = static_cast<int32_t>(
                            (static_cast<int64_t>(rCurrent) * raw(kFractalAngleScale)) >> 16
                        );
                        f16 rPhase(static_cast<uint16_t>(rScaled + p.timeRaw));
                        int32_t angleDelta = static_cast<int32_t>(
                            (static_cast<int64_t>(raw(angleSinF16(rPhase))) * p.angularSpeedRaw) >> 16
                        );
                        totalDTheta += angleDelta;
                        totalDr += raw(kFractalRadiusStep);
                        rCurrent += raw(kFractalRadiusStep);
                    }
                    // Scale radial by speed parameter.
                    totalDr = static_cast<int32_t>((static_cast<int64_t>(totalDr) * p.radialSpeedRaw) >> 16);
                    return polarToCartesian(polar, totalDr, totalDTheta);
                }

                case TransportPattern::TransportMode::SquareSpiral: {
                    // Grid-aligned spiral: determine which arm the pixel is on.
                    int32_t halfRaw = (static_cast<int32_t>(gridSize - 1u) << kQ16Shift) >> 1;
                    int32_t px = (static_cast<int32_t>(x) << kQ16Shift) + (SF16_ONE >> 1) - halfRaw;
                    int32_t py = (static_cast<int32_t>(y) << kQ16Shift) + (SF16_ONE >> 1) - halfRaw;
                    int32_t apx = px < 0 ? -px : px;
                    int32_t apy = py < 0 ? -py : py;

                    // Determine quadrant of the spiral arm based on which edge is closer.
                    // Right→Down→Left→Up in clockwise sense.
                    int32_t speed = p.radialSpeedRaw;
                    v32 disp;
                    if (apx >= apy) {
                        // Horizontal-dominant: right or left edge.
                        disp = (px >= 0)
                            ? v32{0, speed}    // right edge → move down
                            : v32{0, -speed};  // left edge → move up
                    } else {
                        // Vertical-dominant: top or bottom edge.
                        disp = (py >= 0)
                            ? v32{-speed, 0}   // bottom → move left
                            : v32{speed, 0};   // top → move right
                    }
                    return disp;
                }

                case TransportPattern::TransportMode::AttractorField: {
                    int32_t totalDx = 0;
                    int32_t totalDy = 0;
                    int32_t pxRaw = (static_cast<int32_t>(x) << kQ16Shift) + (SF16_ONE >> 1);
                    int32_t pyRaw = (static_cast<int32_t>(y) << kQ16Shift) + (SF16_ONE >> 1);

                    for (uint8_t i = 0; i < kAttractorCount; ++i) {
                        int32_t dx = p.attractorX[i] - pxRaw;
                        int32_t dy = p.attractorY[i] - pyRaw;
                        int64_t dist2 = static_cast<int64_t>(dx) * dx + static_cast<int64_t>(dy) * dy;
                        if (dist2 < kAttractorMinDist2) dist2 = kAttractorMinDist2;

                        // Inverse-square strength: strength = speed / dist².
                        // To avoid overflow: strength_raw = (speed << 16) / (dist2 >> 16).
                        int64_t dist2Shifted = dist2 >> 16;
                        if (dist2Shifted == 0) dist2Shifted = 1;
                        int32_t strength = static_cast<int32_t>(
                            (static_cast<int64_t>(p.radialSpeedRaw) << 16) / dist2Shifted
                        );
                        // Clamp to prevent instability.
                        if (strength > (4 << 16)) strength = 4 << 16;

                        // Radial component (toward attractor).
                        int32_t radX = static_cast<int32_t>((static_cast<int64_t>(dx) * strength) >> 16);
                        int32_t radY = static_cast<int32_t>((static_cast<int64_t>(dy) * strength) >> 16);
                        // Tangential component (perpendicular, CCW).
                        int32_t tanX = static_cast<int32_t>((static_cast<int64_t>(-dy) * strength) >> 16);
                        int32_t tanY = static_cast<int32_t>((static_cast<int64_t>(dx) * strength) >> 16);
                        int32_t swirlRaw = raw(kAttractorSwirlRatio);
                        tanX = static_cast<int32_t>((static_cast<int64_t>(tanX) * swirlRaw) >> 16);
                        tanY = static_cast<int32_t>((static_cast<int64_t>(tanY) * swirlRaw) >> 16);

                        totalDx += radX + tanX;
                        totalDy += radY + tanY;
                    }
                    return v32{totalDx, totalDy};
                }
            }
            return v32{0, 0};
        }
    }

    // ---- State ----

    struct TransportPattern::State {
        uint8_t gridSize;
        TransportMode mode;
        bool velocityGlow;
        std::unique_ptr<uint16_t[]> cells;
        std::unique_ptr<uint16_t[]> scratch;
        Sf16Signal radialSpeedSignal;
        Sf16Signal angularSpeedSignal;
        Sf16Signal halfLifeSignal;
        Sf16Signal emitterSpeedSignal;
        fl::s16x16 radialSpeed{fl::s16x16::from_raw(0)};
        fl::s16x16 angularSpeed{fl::s16x16::from_raw(0)};
        uint16_t halfLifeMs{860u};
        fl::s16x16 emitterSpeed{fl::s16x16::from_raw(0)};
        fl::s16x16 timeQ16{fl::s16x16::from_raw(0)};
        TimeMillis lastElapsedMs{0u};
        bool hasLastElapsed{false};
        // Shockwave state.
        int32_t shockwaveRadiusRaw{0};
        // Attractor phases.
        uint16_t attractorPhases[kAttractorCount * 2]; // x and y phases interleaved

        explicit State(
            uint8_t size,
            TransportMode mode,
            bool glow,
            Sf16Signal radialSpeed,
            Sf16Signal angularSpeed,
            Sf16Signal halfLife,
            Sf16Signal emitterSpeed
        ) : gridSize(size),
            mode(mode),
            velocityGlow(glow),
            cells(std::make_unique<uint16_t[]>(static_cast<size_t>(size) * size)),
            scratch(std::make_unique<uint16_t[]>(static_cast<size_t>(size) * size)),
            radialSpeedSignal(std::move(radialSpeed)),
            angularSpeedSignal(std::move(angularSpeed)),
            halfLifeSignal(std::move(halfLife)),
            emitterSpeedSignal(std::move(emitterSpeed)) {
            std::fill_n(cells.get(), static_cast<size_t>(size) * size, uint16_t(0));
            std::fill_n(scratch.get(), static_cast<size_t>(size) * size, uint16_t(0));

            // Attractor initial phases.
            for (uint8_t i = 0; i < kAttractorCount * 2; ++i) {
                attractorPhases[i] = static_cast<uint16_t>(random32());
            }
        }
    };

    // ---- Functor ----

    struct TransportPattern::TransportFunctor {
        const State *state;

        PatternNormU16 operator()(UV uv) const {
            return sampleScalarGridClamped(state->cells.get(), state->gridSize, state->gridSize, uv);
        }
    };

    // ---- Constructor ----

    TransportPattern::TransportPattern(
        uint8_t gridSize,
        TransportMode mode,
        Sf16Signal radialSpeed,
        Sf16Signal angularSpeed,
        Sf16Signal halfLife,
        Sf16Signal emitterSpeed,
        bool velocityGlow
    ) : state(std::make_shared<State>(
        std::max<uint8_t>(kMinGridSize, std::min<uint8_t>(gridSize, kMaxGridSize)),
        mode,
        velocityGlow,
        std::move(radialSpeed),
        std::move(angularSpeed),
        std::move(halfLife),
        std::move(emitterSpeed)
    )) {
    }

    // ---- advanceFrame ----

    void TransportPattern::advanceFrame(f16 progress, TimeMillis elapsedMs) {
        (void)progress;
        State &s = *state;

        // 1. Delta time.
        TimeMillis dtMs = 0u;
        if (!s.hasLastElapsed) {
            s.lastElapsedMs = elapsedMs;
            s.hasLastElapsed = true;
        } else {
            dtMs = clampDeltaTime(elapsedMs - s.lastElapsedMs);
            s.lastElapsedMs = elapsedMs;
            s.timeQ16 = s.timeQ16 + fl::s16x16::from_raw(raw(timeMillisToScalar(dtMs)));
        }

        // 2. Sample signals.
        s.radialSpeed = s.radialSpeedSignal.sample(radialSpeedRange(), elapsedMs);
        s.angularSpeed = s.angularSpeedSignal.sample(angularSpeedRange(), elapsedMs);
        s.emitterSpeed = s.emitterSpeedSignal.sample(emitterSpeedRange(), elapsedMs);

        {
            uint16_t halfLifeT = raw(s.halfLifeSignal.sample(magnitudeRange(), elapsedMs));
            uint32_t range = kHalfLifeMaxMs - kHalfLifeMinMs;
            s.halfLifeMs = static_cast<uint16_t>(kHalfLifeMinMs + ((static_cast<uint32_t>(halfLifeT) * range) >> 16));
        }

        // 3. Mode-specific per-frame updates.
        uint8_t gridSize = s.gridSize;

        if (s.mode == TransportMode::Shockwave) {
            // Expand wavefront. Reset when it exceeds grid diagonal.
            int32_t maxRadius = static_cast<int32_t>(gridSize) << (kQ16Shift - 1); // half grid
            s.shockwaveRadiusRaw += raw(s.radialSpeed);
            if (s.shockwaveRadiusRaw > maxRadius) {
                s.shockwaveRadiusRaw = 0;
            }
        }

        // 4. Inject emitters (Lissajous line onto grid).
        uint16_t *cells = s.cells.get();
        fl::s16x16 gridCenter = fl::s16x16::from_raw(static_cast<int32_t>(gridSize - 1u) * raw(kGridHalf));
        emitLissajousLine(cells, gridSize, gridCenter, s.timeQ16, s.emitterSpeed);

        // 5. Build transport params and advect.
        TransportParams tp{};
        tp.mode = s.mode;
        tp.gridSize = gridSize;
        tp.radialSpeedRaw = raw(s.radialSpeed);
        tp.angularSpeedRaw = raw(s.angularSpeed);
        tp.timeRaw = raw(s.timeQ16);
        tp.shockwaveRadiusRaw = s.shockwaveRadiusRaw;

        // Compute attractor positions for AttractorField mode.
        if (s.mode == TransportMode::AttractorField) {
            fl::s16x16 gc = gridCenter;
            fl::s16x16 orbRadius = scaleByGridSize(gridSize, s16x16FromFraction(3, 8));
            for (uint8_t i = 0; i < kAttractorCount; ++i) {
                f16 phX(static_cast<uint16_t>(raw(s.timeQ16 * s.emitterSpeed) + s.attractorPhases[i * 2]));
                f16 phY(static_cast<uint16_t>(raw(s.timeQ16 * s.emitterSpeed) + s.attractorPhases[i * 2 + 1]));
                tp.attractorX[i] = raw(gc) + static_cast<int32_t>(
                    (static_cast<int64_t>(raw(orbRadius)) * raw(angleSinF16(phX))) >> 16
                );
                tp.attractorY[i] = raw(gc) + static_cast<int32_t>(
                    (static_cast<int64_t>(raw(orbRadius)) * raw(angleCosF16(phY))) >> 16
                );
            }
        }

        f16 fadeFactor = halfLifeFade(dtMs, s.halfLifeMs);
        uint16_t *scratch = s.scratch.get();

        advectGrid2DBackward(cells, scratch, gridSize, transportVector, &tp, fadeFactor);

        // Swap cells and scratch.
        s.cells.swap(s.scratch);

        // 6. Velocity glow: add brightness proportional to displacement magnitude.
        if (s.velocityGlow) {
            uint16_t *result = s.cells.get();
            for (uint8_t y = 0; y < gridSize; ++y) {
                for (uint8_t x = 0; x < gridSize; ++x) {
                    v32 disp = transportVector(x, y, gridSize, &tp);
                    uint64_t mag2 = static_cast<uint64_t>(
                        static_cast<int64_t>(disp.x) * disp.x + static_cast<int64_t>(disp.y) * disp.y
                    );
                    uint32_t mag = static_cast<uint32_t>(sqrtU64Raw(mag2));
                    // Scale magnitude to brightness addition (cap at ~1/4 full).
                    uint16_t glow = static_cast<uint16_t>(std::min<uint32_t>(mag >> 2, kFullIntensity >> 2));
                    size_t idx = static_cast<size_t>(y) * gridSize + x;
                    uint32_t sum = static_cast<uint32_t>(result[idx]) + glow;
                    result[idx] = static_cast<uint16_t>(sum > kFullIntensity ? kFullIntensity : sum);
                }
            }
        }
    }

    // ---- layer ----

    UVMap TransportPattern::layer(const std::shared_ptr<PipelineContext> &context) const {
        (void)context;
        return TransportFunctor{state.get()};
    }
}
