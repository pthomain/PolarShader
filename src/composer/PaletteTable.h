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

#ifndef POLAR_SHADER_COMPOSER_PALETTE_TABLE_H
#define POLAR_SHADER_COMPOSER_PALETTE_TABLE_H

#if defined(ARDUINO) || defined(__EMSCRIPTEN__)
#include "FastLED.h"
#else
#include "native/FastLED.h"
#endif

namespace PolarShader::composer {
    // Returns the palette pointer for the given composer-local palette ID,
    // or nullptr if the ID is unknown.
    //
    // Production implementation: src/composer/PaletteTable.cpp.
    // Test implementation: test/test_composer/PaletteTable_testimpl.cpp,
    // substituted into the composer test env via build_src_filter.
    //
    // The wire format and the JS schema (web/sketches/composer/schema.js)
    // both reference these IDs. Adding a palette = entry in production
    // PaletteTable.cpp, test PaletteTable_testimpl.cpp, and schema.js.
    const ::CRGBPalette16 *paletteById(uint8_t id);
}

#endif // POLAR_SHADER_COMPOSER_PALETTE_TABLE_H
