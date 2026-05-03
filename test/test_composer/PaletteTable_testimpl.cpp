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

// Test-only replacement for src/composer/PaletteTable.cpp.
// The composer test env's build_src_filter excludes the production cpp and
// pulls this one in instead, so test_composer's render-equality fixtures
// see nontrivial palettes (the native FastLED stubs in
// src/native/FastLED.h are all-black placeholders).
//
// IDs match the production table.

#include "composer/PaletteTable.h"

namespace PolarShader::composer {
    namespace {
        // Nontrivial palettes built by mutating the default-constructed
        // entries array (the native CRGBPalette16 stub only exposes the
        // default constructor — we don't widen the stub). Distinct
        // palette IDs map to distinguishable CRGB outputs in tests.

        struct PaletteSeed {
            uint8_t base;
            uint8_t step;
            uint8_t channelOffset[3];
        };

        // Build a 16-entry palette by sweeping a base color along the
        // channels with a fixed step. Differs across IDs so distinct
        // palettes produce distinct CRGB outputs at the same index.
        ::CRGBPalette16 buildSeeded(const PaletteSeed &seed) {
            ::CRGBPalette16 p;
            for (int i = 0; i < 16; ++i) {
                uint8_t r = static_cast<uint8_t>(seed.base + seed.channelOffset[0] + i * seed.step);
                uint8_t g = static_cast<uint8_t>(seed.base + seed.channelOffset[1] + i * seed.step);
                uint8_t b = static_cast<uint8_t>(seed.base + seed.channelOffset[2] + i * seed.step);
                p.entries[i] = ::CRGB(r, g, b);
            }
            return p;
        }

        const ::CRGBPalette16 &kPalette0() {
            static const ::CRGBPalette16 p = buildSeeded({0x10, 0x09, {0x80, 0x40, 0x20}});
            return p;
        }
        const ::CRGBPalette16 &kPalette1() {
            static const ::CRGBPalette16 p = buildSeeded({0x20, 0x07, {0x10, 0x50, 0x90}});
            return p;
        }
        const ::CRGBPalette16 &kPalette2() {
            static const ::CRGBPalette16 p = buildSeeded({0x30, 0x05, {0xA0, 0x10, 0x70}});
            return p;
        }
        const ::CRGBPalette16 &kPalette3() {
            static const ::CRGBPalette16 p = buildSeeded({0x40, 0x03, {0x20, 0xA0, 0x30}});
            return p;
        }
    }

    const ::CRGBPalette16 *paletteById(uint8_t id) {
        switch (id) {
            case 0: return &kPalette0();
            case 1: return &kPalette1();
            case 2: return &kPalette2();
            case 3: return &kPalette3();
            default: return nullptr;
        }
    }
}
