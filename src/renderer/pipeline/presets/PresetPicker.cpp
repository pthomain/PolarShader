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

#include "PresetPicker.h"
#include <FastLED.h>
#include "renderer/pipeline/patterns/Patterns.h"

namespace PolarShader {
    namespace {
        Layer makeHexKaleidoscope(const CRGBPalette16 &palette) {
            return hexKaleidoscopePreset(palette).build();
        }
        
        Layer makeNoiseKaleidoscope(const CRGBPalette16 &palette) {
            return noiseKaleidoscopePattern(palette).build();
        }
    }

    Layer PresetPicker::pickRandom(const CRGBPalette16 &palette) {
        static const PipelineFactory factories[] = {
            makeHexKaleidoscope,
            makeNoiseKaleidoscope
        };

        uint8_t idx = random8(std::size(factories));
        return factories[idx](palette);
    }
}
