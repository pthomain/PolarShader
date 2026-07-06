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

// Production palette ID table.
// Substituted in the composer test env by
// test/test_composer/PaletteTable_testimpl.cpp.

#include "composer/PaletteTable.h"

namespace PolarShader::composer {
    namespace {
        struct Rgb {
            uint8_t r;
            uint8_t g;
            uint8_t b;
        };

        ::CRGBPalette16 buildPalette(const Rgb (&entries)[16]) {
            ::CRGBPalette16 p;
            for (uint8_t i = 0; i < 16; ++i) {
                p.entries[i] = ::CRGB(entries[i].r, entries[i].g, entries[i].b);
            }
            return p;
        }

        // Keep the composer palettes as ordinary RAM CRGBPalette16 values.
        // FastLED's stock palettes are PROGMEM arrays; constructing from them
        // inside the Emscripten worker can trap when live scene edits first
        // materialise a non-default palette.
        const ::CRGBPalette16 &kPaletteRainbow() {
            static constexpr Rgb entries[16] = {
                {0xFF, 0x00, 0x00}, {0xD5, 0x2A, 0x00}, {0xAB, 0x55, 0x00}, {0xAB, 0x7F, 0x00},
                {0xAB, 0xAB, 0x00}, {0x56, 0xD5, 0x00}, {0x00, 0xFF, 0x00}, {0x00, 0xD5, 0x2A},
                {0x00, 0xAB, 0x55}, {0x00, 0x56, 0xAA}, {0x00, 0x00, 0xFF}, {0x2A, 0x00, 0xD5},
                {0x55, 0x00, 0xAB}, {0x7F, 0x00, 0x81}, {0xAB, 0x00, 0x55}, {0xD5, 0x00, 0x2B},
            };
            static const ::CRGBPalette16 p = buildPalette(entries);
            return p;
        }

        const ::CRGBPalette16 &kPaletteCloud() {
            static constexpr Rgb entries[16] = {
                {0x00, 0x00, 0xFF}, {0x00, 0x00, 0x8B}, {0x00, 0x00, 0x8B}, {0x00, 0x00, 0x8B},
                {0x00, 0x00, 0x8B}, {0x00, 0x00, 0x8B}, {0x00, 0x00, 0x8B}, {0x00, 0x00, 0x8B},
                {0x00, 0x00, 0xFF}, {0x00, 0x00, 0x8B}, {0x87, 0xCE, 0xEB}, {0x87, 0xCE, 0xEB},
                {0xAD, 0xD8, 0xE6}, {0xFF, 0xFF, 0xFF}, {0xAD, 0xD8, 0xE6}, {0x87, 0xCE, 0xEB},
            };
            static const ::CRGBPalette16 p = buildPalette(entries);
            return p;
        }

        const ::CRGBPalette16 &kPaletteParty() {
            static constexpr Rgb entries[16] = {
                {0x55, 0x00, 0xAB}, {0x84, 0x00, 0x7C}, {0xB5, 0x00, 0x4B}, {0xE5, 0x00, 0x1B},
                {0xE8, 0x17, 0x00}, {0xB8, 0x47, 0x00}, {0xAB, 0x77, 0x00}, {0xAB, 0xAB, 0x00},
                {0xAB, 0x55, 0x00}, {0xDD, 0x22, 0x00}, {0xF2, 0x00, 0x0E}, {0xC2, 0x00, 0x3E},
                {0x8F, 0x00, 0x71}, {0x5F, 0x00, 0xA1}, {0x2F, 0x00, 0xD0}, {0x00, 0x07, 0xF9},
            };
            static const ::CRGBPalette16 p = buildPalette(entries);
            return p;
        }

        const ::CRGBPalette16 &kPaletteForest() {
            static constexpr Rgb entries[16] = {
                {0x00, 0x64, 0x00}, {0x00, 0x64, 0x00}, {0x55, 0x6B, 0x2F}, {0x00, 0x64, 0x00},
                {0x00, 0x80, 0x00}, {0x22, 0x8B, 0x22}, {0x6B, 0x8E, 0x23}, {0x00, 0x80, 0x00},
                {0x2E, 0x8B, 0x57}, {0x66, 0xCD, 0xAA}, {0x32, 0xCD, 0x32}, {0x9A, 0xCD, 0x32},
                {0x90, 0xEE, 0x90}, {0x7C, 0xFC, 0x00}, {0x66, 0xCD, 0xAA}, {0x22, 0x8B, 0x22},
            };
            static const ::CRGBPalette16 p = buildPalette(entries);
            return p;
        }
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
