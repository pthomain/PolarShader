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

#ifndef POLARSHADER_DISPLAY_FASTLED_H
#define POLARSHADER_DISPLAY_FASTLED_H

// Standalone shim: do NOT delegate to src/FastLED.h, because src/FastLED.h is
// excluded from the WASM staging dir (its #include_next has nothing to find
// when the stage dir is the last -I entry). Native and Arduino paths only.
#if defined(ARDUINO) || defined(__EMSCRIPTEN__)
#include <FastLED.h>
#if defined(__EMSCRIPTEN__) && !defined(D1)
#define D1 1
#endif
#else
#include "../native/FastLED.h"
#endif

#endif // POLARSHADER_DISPLAY_FASTLED_H
