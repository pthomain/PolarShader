// Vendored 16-bit Perlin noise from FastLED 3.10.4 (src/noise.h, src/noise.cpp.hpp).
//
// FastLED is distributed under the MIT License:
//
//   The MIT License (MIT)
//   Copyright (c) 2013 FastLED
//
//   Permission is hereby granted, free of charge, to any person obtaining a copy
//   of this software and associated documentation files (the "Software"), to deal
//   in the Software without restriction, including without limitation the rights
//   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//   copies of the Software, and to permit persons to whom the Software is
//   furnished to do so, subject to the following conditions:
//
//   The above copyright notice and this permission notice shall be included in
//   all copies or substantial portions of the Software.
//
//   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//   SOFTWARE.
//
// The MIT License is compatible with PolarShader's GPL-3.0-or-later license.
//
// This unit is a self-contained extraction of FastLED's fixed-point inoise16
// (1D/2D/3D) with the FASTLED_NOISE_FIXED=1 / FASTLED_SCALE8_FIXED=1 build
// configuration baked in. Every signature and internal type is plain
// <cstdint> fixed-width, matching the native stub's inoise16 surface so the
// exporter build can swap the fake hash for the real algorithm bit-for-bit.

#ifndef POLAR_SHADER_TOOLS_THIRDPARTY_REAL_NOISE16_H
#define POLAR_SHADER_TOOLS_THIRDPARTY_REAL_NOISE16_H

#include <cstdint>

uint16_t inoise16(uint32_t x);
uint16_t inoise16(uint32_t x, uint32_t y);
uint16_t inoise16(uint32_t x, uint32_t y, uint32_t z);

#endif // POLAR_SHADER_TOOLS_THIRDPARTY_REAL_NOISE16_H
