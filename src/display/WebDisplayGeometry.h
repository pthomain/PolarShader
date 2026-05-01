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
#include "RoundDisplaySpec.h"

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

    inline WebDisplayGeometry buildRoundWebGeometry(const RoundDisplaySpec &spec = RoundDisplaySpec()) {
        WebDisplayGeometry geometry;
        const float maxRadius = static_cast<float>(spec.numSegments() > 0 ? spec.numSegments() - 1 : 0);
        geometry.centerX = maxRadius;
        geometry.centerY = maxRadius;
        geometry.points.reserve(spec.nbLeds());

        for (uint16_t segmentIndex = 0; segmentIndex < spec.numSegments(); ++segmentIndex) {
            const uint16_t segmentLength = spec.segmentSize(segmentIndex);
            const float radius = static_cast<float>(segmentIndex);

            if (segmentLength == 0) {
                continue;
            }

            if (segmentIndex == 0) {
                geometry.points.push_back({geometry.centerX, geometry.centerY});
                continue;
            }

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

    inline WebDisplayGeometry buildWebGeometry(const FabricDisplaySpec &) {
        return buildFabricWebGeometry();
    }

    inline WebDisplayGeometry buildWebGeometry(const RoundDisplaySpec &spec) {
        return buildRoundWebGeometry(spec);
    }
}

#endif // POLARSHADER_WEBDISPLAYGEOMETRY_H
