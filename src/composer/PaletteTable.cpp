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
        struct Entry {
            uint8_t id;
            const ::CRGBPalette16 *palette;
        };

        // `static const`, not `static constexpr`: FastLED palette globals'
        // types and constexpr-correctness vary across versions and platforms.
        // `const` avoids tripping over that.
        static const Entry kPalettes[] = {
            {0, &Rainbow_gp},
            {1, &CloudColors_p},
            {2, &PartyColors_p},
            {3, &ForestColors_p},
        };
    }

    const ::CRGBPalette16 *paletteById(uint8_t id) {
        for (const auto &entry : kPalettes) {
            if (entry.id == id) return entry.palette;
        }
        return nullptr;
    }
}
