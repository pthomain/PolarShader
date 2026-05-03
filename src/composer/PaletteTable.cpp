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

// Production palette ID table — pointers to FastLED palette globals.
// Substituted in the composer test env by
// test/test_composer/PaletteTable_testimpl.cpp.

#include "composer/PaletteTable.h"

namespace PolarShader::composer {
    namespace {
        // FastLED's palette globals come in two flavours:
        //   - `..._gp` suffix → TProgmemRGBGradientPalette_byte[] (color stops)
        //   - `..._p`  suffix → TProgmemRGBPalette16 (16 raw colors)
        // Neither is a CRGBPalette16; both convert via CRGBPalette16's
        // converting constructors. We keep one CRGBPalette16 instance per
        // ID, materialised once at first access, and return a pointer.
        const ::CRGBPalette16 &kPaletteRainbow() { static const ::CRGBPalette16 p(Rainbow_gp);      return p; }
        const ::CRGBPalette16 &kPaletteCloud()   { static const ::CRGBPalette16 p(CloudColors_p);   return p; }
        const ::CRGBPalette16 &kPaletteParty()   { static const ::CRGBPalette16 p(PartyColors_p);   return p; }
        const ::CRGBPalette16 &kPaletteForest()  { static const ::CRGBPalette16 p(ForestColors_p);  return p; }
    }

    const ::CRGBPalette16 *paletteById(uint8_t id) {
        switch (id) {
            case 0: return &kPaletteRainbow();
            case 1: return &kPaletteCloud();
            case 2: return &kPaletteParty();
            case 3: return &kPaletteForest();
            default: return nullptr;
        }
    }
}
