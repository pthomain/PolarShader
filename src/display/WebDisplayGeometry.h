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

#ifndef POLARSHADER_WEBDISPLAYGEOMETRY_H
#define POLARSHADER_WEBDISPLAYGEOMETRY_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <vector>

#include "FabricDisplaySpec.h"
#include "Fabric32x8DisplaySpec.h"
#include "FibonacciDisplaySpec.h"
#include "Matrix128x128DisplaySpec.h"
#include "RoundDisplaySpec.h"
#include "display/LoadedDisplaySpec.h"
#include "renderer/pipeline/maths/units/Units.h"

namespace PolarShader {
    struct WebDisplayPoint {
        float x;
        float y;
    };

    struct WebDisplayGeometry {
        std::vector<WebDisplayPoint> points;
        float diameter{0.0f};
        float centerX{0.0f};
        float centerY{0.0f};
    };

    namespace detail {
        constexpr float DEFAULT_LED_FILL = 0.8f;
        constexpr float PI_F = 3.14159265358979323846f;

        inline float squaredDistance(const WebDisplayPoint &lhs, const WebDisplayPoint &rhs) {
            const float dx = lhs.x - rhs.x;
            const float dy = lhs.y - rhs.y;
            return (dx * dx) + (dy * dy);
        }

        inline float nearestNeighbourSpacing(const std::vector<WebDisplayPoint> &points) {
            if (points.size() < 2) {
                return 1.0f;
            }

            float minSquaredDistance = std::numeric_limits<float>::max();
            for (std::size_t i = 0; i < points.size(); ++i) {
                for (std::size_t j = i + 1; j < points.size(); ++j) {
                    const float squaredDistance = PolarShader::detail::squaredDistance(points[i], points[j]);
                    if (squaredDistance > 0.0f && squaredDistance < minSquaredDistance) {
                        minSquaredDistance = squaredDistance;
                    }
                }
            }

            if (minSquaredDistance == std::numeric_limits<float>::max()) {
                return 1.0f;
            }

            return std::sqrt(minSquaredDistance);
        }

        inline float deriveLedDiameter(const std::vector<WebDisplayPoint> &points) {
            return nearestNeighbourSpacing(points) * DEFAULT_LED_FILL;
        }
    }

    inline WebDisplayGeometry buildFabricWebGeometry() {
        WebDisplayGeometry geometry;
        geometry.points.resize(FabricDisplaySpec::WIDTH * FabricDisplaySpec::HEIGHT);
        geometry.centerX = static_cast<float>(FabricDisplaySpec::WIDTH - 1) * 0.5f;
        geometry.centerY = static_cast<float>(FabricDisplaySpec::HEIGHT - 1) * 0.5f;

        for (uint16_t row = 0; row < FabricDisplaySpec::HEIGHT; ++row) {
            for (uint16_t col = 0; col < FabricDisplaySpec::WIDTH; ++col) {
                const uint16_t indexInRow = row & 1 ? (FabricDisplaySpec::WIDTH - 1) - col : col;
                const uint16_t pixelIndex = static_cast<uint16_t>((row * FabricDisplaySpec::WIDTH) + indexInRow);
                geometry.points[pixelIndex] = {
                    static_cast<float>(col),
                    static_cast<float>(row)
                };
            }
        }

        geometry.diameter = detail::deriveLedDiameter(geometry.points);
        return geometry;
    }

    inline WebDisplayGeometry buildFabric32x8WebGeometry() {
        WebDisplayGeometry geometry;
        geometry.points.resize(Fabric32x8DisplaySpec::WIDTH * Fabric32x8DisplaySpec::HEIGHT);
        geometry.centerX = static_cast<float>(Fabric32x8DisplaySpec::WIDTH - 1) * 0.5f;
        geometry.centerY = static_cast<float>(Fabric32x8DisplaySpec::HEIGHT - 1) * 0.5f;

        // Vertical serpentine: even columns run top-to-bottom, odd columns
        // bottom-to-top. Must match Fabric32x8DisplaySpec's physical mapping.
        for (uint16_t col = 0; col < Fabric32x8DisplaySpec::WIDTH; ++col) {
            for (uint16_t p = 0; p < Fabric32x8DisplaySpec::HEIGHT; ++p) {
                const uint16_t row = col & 1 ? (Fabric32x8DisplaySpec::HEIGHT - 1) - p : p;
                const uint16_t pixelIndex = static_cast<uint16_t>((col * Fabric32x8DisplaySpec::HEIGHT) + p);
                geometry.points[pixelIndex] = {
                    static_cast<float>(col),
                    static_cast<float>(row)
                };
            }
        }

        geometry.diameter = detail::deriveLedDiameter(geometry.points);
        return geometry;
    }

    inline WebDisplayGeometry buildMatrixWebGeometry(const MatrixDisplaySpec &spec) {
        WebDisplayGeometry geometry;
        const uint16_t width = spec.matrixWidth();
        const uint16_t height = spec.matrixHeight();
        geometry.points.resize(static_cast<std::size_t>(width) * height);
        geometry.centerX = static_cast<float>(width > 0 ? width - 1 : 0) * 0.5f;
        geometry.centerY = static_cast<float>(height > 0 ? height - 1 : 0) * 0.5f;

        for (uint16_t row = 0; row < height; ++row) {
            for (uint16_t col = 0; col < width; ++col) {
                const uint16_t pixelIndex = static_cast<uint16_t>((row * width) + col);
                geometry.points[pixelIndex] = {
                    static_cast<float>(col),
                    static_cast<float>(row)
                };
            }
        }

        geometry.diameter = detail::deriveLedDiameter(geometry.points);
        return geometry;
    }

    inline WebDisplayGeometry buildRoundWebGeometry(const RoundDisplaySpec &spec = RoundDisplaySpec()) {
        WebDisplayGeometry geometry;
        const float maxRadius = static_cast<float>(spec.numSegments() > 0 ? spec.numSegments() - 1 : 0);
        geometry.centerX = maxRadius;
        geometry.centerY = maxRadius;
        geometry.points.reserve(spec.nbLeds());

        for (uint16_t segmentIndex = 0; segmentIndex < spec.numSegments(); ++segmentIndex) {
            const uint16_t segmentLength = spec.segmentSize(segmentIndex);
            const float radius = static_cast<float>(segmentIndex);

            // Always emit `segmentLength` points so geometry.points.size()
            // matches spec.nbLeds(). Segment 0 yields coincident points at
            // the center because radius == 0.
            for (uint16_t pixelInSegment = 0; pixelInSegment < segmentLength; ++pixelInSegment) {
                const float angle = (2.0f * detail::PI_F * static_cast<float>(pixelInSegment))
                                    / static_cast<float>(segmentLength);
                geometry.points.push_back({
                    geometry.centerX + (std::cos(angle) * radius),
                    geometry.centerY + (std::sin(angle) * radius)
                });
            }
        }

        geometry.diameter = detail::deriveLedDiameter(geometry.points);
        return geometry;
    }

    inline WebDisplayGeometry buildFibonacciWebGeometry(const FibonacciDisplaySpec &spec = FibonacciDisplaySpec()) {
        WebDisplayGeometry geometry;
        // Rim radius maps to the physical spiral end; centre placed so all
        // points fall in [0, 2*RADIUS_MAX_MM]. Preserves log-spiral spacing.
        const float rimRadius = FibonacciDisplaySpec::RADIUS_MAX_MM;
        geometry.centerX = rimRadius;
        geometry.centerY = rimRadius;
        geometry.points.reserve(spec.nbLeds());

        for (uint16_t pixelIndex = 0; pixelIndex < spec.nbLeds(); ++pixelIndex) {
            const PolarCoords coords = spec.toPolarCoords(pixelIndex);
            const float angle = (2.0f * detail::PI_F * static_cast<float>(raw(coords.first)))
                                / 65536.0f;
            const float radius = (static_cast<float>(raw(coords.second)) / 65535.0f) * rimRadius;
            // Screen convention (y-down): increasing angle winds clockwise,
            // matching the approved layout preview.
            geometry.points.push_back({
                geometry.centerX + (std::cos(angle) * radius),
                geometry.centerY + (std::sin(angle) * radius)
            });
        }

        geometry.diameter = detail::deriveLedDiameter(geometry.points);
        return geometry;
    }

    // Runtime-loaded (.PDS) display. Mirrors the built-ins: raster displays lay
    // out on their rectangular (col, row) grid; polar-only displays project
    // (angle, radius) → cartesian like Fibonacci.
    inline WebDisplayGeometry buildLoadedWebGeometry(const LoadedDisplaySpec &spec) {
        WebDisplayGeometry geometry;
        geometry.points.reserve(spec.nbLeds());

        if (spec.hasRaster) {
            geometry.centerX = static_cast<float>(spec.rasterWidth > 0 ? spec.rasterWidth - 1 : 0) * 0.5f;
            geometry.centerY = static_cast<float>(spec.rasterHeight > 0 ? spec.rasterHeight - 1 : 0) * 0.5f;
            for (const auto &cell : spec.rasterCells) {
                geometry.points.push_back({
                    static_cast<float>(cell.x),
                    static_cast<float>(cell.y)
                });
            }
        } else {
            // Unit-rim polar projection (absolute scale is irrelevant — the LED
            // diameter is derived from nearest-neighbour spacing).
            constexpr float rimRadius = 100.0f;
            geometry.centerX = rimRadius;
            geometry.centerY = rimRadius;
            for (const auto &led : spec.leds) {
                const float angle = (2.0f * detail::PI_F * static_cast<float>(raw(led.first))) / 65536.0f;
                const float radius = (static_cast<float>(raw(led.second)) / 65535.0f) * rimRadius;
                geometry.points.push_back({
                    geometry.centerX + (std::cos(angle) * radius),
                    geometry.centerY + (std::sin(angle) * radius)
                });
            }
        }

        geometry.diameter = detail::deriveLedDiameter(geometry.points);
        return geometry;
    }

    inline WebDisplayGeometry buildWebGeometry(const LoadedDisplaySpec &spec) {
        return buildLoadedWebGeometry(spec);
    }

    inline WebDisplayGeometry buildWebGeometry(const FabricDisplaySpec &) {
        return buildFabricWebGeometry();
    }

    inline WebDisplayGeometry buildWebGeometry(const Fabric32x8DisplaySpec &) {
        return buildFabric32x8WebGeometry();
    }

    inline WebDisplayGeometry buildWebGeometry(const Matrix128x128DisplaySpec &spec) {
        return buildMatrixWebGeometry(spec);
    }

    inline WebDisplayGeometry buildWebGeometry(const RoundDisplaySpec &spec) {
        return buildRoundWebGeometry(spec);
    }

    inline WebDisplayGeometry buildWebGeometry(const FibonacciDisplaySpec &spec) {
        return buildFibonacciWebGeometry(spec);
    }
}

#endif // POLARSHADER_WEBDISPLAYGEOMETRY_H
