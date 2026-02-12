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

#include "Patterns.h"
#include "NoisePattern.h"
#include "HexTilingPattern.h"

namespace PolarShader {
    std::unique_ptr<UVPattern> worleyPattern(
        sr8 cellSize,
        WorleyAliasing aliasingMode
    ) {
        return std::make_unique<WorleyPattern>(cellSize, aliasingMode);
    }

    std::unique_ptr<UVPattern> voronoiPattern(
        sr8 cellSize,
        WorleyAliasing aliasingMode
    ) {
        return std::make_unique<VoronoiPattern>(cellSize, aliasingMode);
    }

    std::unique_ptr<UVPattern> noisePattern() {
        return std::make_unique<NoisePattern>(NoisePattern::NoiseType::Basic);
    }

    std::unique_ptr<UVPattern> fbmNoisePattern(fl::u8 octaves) {
        return std::make_unique<NoisePattern>(NoisePattern::NoiseType::FBM, octaves);
    }

    std::unique_ptr<UVPattern> turbulenceNoisePattern() {
        return std::make_unique<NoisePattern>(NoisePattern::NoiseType::Turbulence);
    }

    std::unique_ptr<UVPattern> ridgedNoisePattern() {
        return std::make_unique<NoisePattern>(NoisePattern::NoiseType::Ridged);
    }

    std::unique_ptr<UVPattern> hexTilingPattern(
        uint16_t hexRadius,
        uint8_t colorCount,
        uint16_t edgeSoftness
    ) {
        return std::make_unique<HexTilingPattern>(hexRadius, colorCount, edgeSoftness);
    }
}
