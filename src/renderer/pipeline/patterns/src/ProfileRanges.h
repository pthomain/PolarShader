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

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_SRC_PROFILE_RANGES_H
#define POLAR_SHADER_PIPELINE_PATTERNS_SRC_PROFILE_RANGES_H

// Shared profile constants/ranges used by FlowFieldPattern and FlurryPattern.
// Wrapped in an anonymous namespace so each separately-compiled TU keeps its
// own internal-linkage copy on embedded builds, while the WASM unity build
// (web/build_site.py amalgamates project sources into one .ino) sees only one
// definition thanks to the include guard.

#include "renderer/pipeline/patterns/GridUtils.h"
#include "renderer/pipeline/signals/ranges/BipolarRange.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"

namespace PolarShader {
    namespace {
        constexpr fl::s16x16 kMinFrequency = grid::s16x16FromFraction(1, 64);
        constexpr fl::s16x16 kMaxProfileSpeed = grid::s16x16FromFraction(1, 4);
        constexpr f16 kMaxProfileAmplitude = toF16(1, 2);
        constexpr fl::s16x16 kMaxProfileFrequency = grid::s16x16FromFraction(1, 2);

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
            static const MagnitudeRange range(grid::s16x16FromFraction(0, 1), kMaxProfileSpeed);
            return range;
        }
    }
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_SRC_PROFILE_RANGES_H
