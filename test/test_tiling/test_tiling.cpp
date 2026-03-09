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

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif
#include <unity.h>
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/transforms/TilingTransform.h"

#ifndef ARDUINO
#include "renderer/pipeline/signals/src/Signals.cpp"
#include "renderer/pipeline/signals/src/SignalSamplers.cpp"
#include "renderer/pipeline/signals/src/accumulators/Accumulators.cpp"
#include "renderer/pipeline/maths/src/TilingMaths.cpp"
#include "renderer/pipeline/transforms/src/TilingTransform.cpp"
#endif

using namespace PolarShader;

void test_square_tiling_basic() {
    TilingTransform transform(64, false, TilingTransform::TileShape::SQUARE);
    
    UV captured_uv;
    auto mock_layer = [&](UV uv) { 
        captured_uv = uv;
        return PatternNormU16(0); 
    };
    
    UVMap map(mock_layer);
    auto tiled = transform(map);

    tiled(UV(fl::s16x16::from_raw(0), fl::s16x16::from_raw(0)));
    auto expected = TilingMaths::sampleTile(0, 0, 64, TilingTransform::TileShape::SQUARE);
    TEST_ASSERT_EQUAL_INT32(raw(CartesianMaths::to_uv(fl::s24x8::from_raw(expected.local_x))), raw(captured_uv.u));
    TEST_ASSERT_EQUAL_INT32(raw(CartesianMaths::to_uv(fl::s24x8::from_raw(expected.local_y))), raw(captured_uv.v));
}

void test_triangle_tiling_basic() {
    TilingTransform transform(64, false, TilingTransform::TileShape::TRIANGLE);
    
    UV cap;
    auto mock = [&](UV uv) { cap = uv; return PatternNormU16(0); };
    auto tiled = transform(mock);

    tiled(UV(fl::s16x16::from_raw(0), fl::s16x16::from_raw(0)));
    auto expected = TilingMaths::sampleTile(0, 0, 64, TilingTransform::TileShape::TRIANGLE);
    TEST_ASSERT_EQUAL_INT32(raw(CartesianMaths::to_uv(fl::s24x8::from_raw(expected.local_x))), raw(cap.u));
    TEST_ASSERT_EQUAL_INT32(raw(CartesianMaths::to_uv(fl::s24x8::from_raw(expected.local_y))), raw(cap.v));
}

void test_hexagon_tiling_basic() {
    TilingTransform transform(64, false, TilingTransform::TileShape::HEXAGON);
    
    UV cap;
    auto mock = [&](UV uv) { cap = uv; return PatternNormU16(0); };
    auto tiled = transform(mock);
    
    tiled(UV(fl::s16x16::from_raw(0), fl::s16x16::from_raw(0)));
    auto expected = TilingMaths::sampleTile(0, 0, 64, TilingTransform::TileShape::HEXAGON);
    TEST_ASSERT_EQUAL_INT32(raw(CartesianMaths::to_uv(fl::s24x8::from_raw(expected.local_x))), raw(cap.u));
    TEST_ASSERT_EQUAL_INT32(raw(CartesianMaths::to_uv(fl::s24x8::from_raw(expected.local_y))), raw(cap.v));
}

void test_signal_tiling_basic() {
    TilingTransform transform(constant(128)); 
    
    UV cap;
    auto mock = [&](UV uv) { cap = uv; return PatternNormU16(0); };
    auto tiled = transform(mock);
    
    tiled(UV(fl::s16x16::from_raw(0), fl::s16x16::from_raw(0)));
    int32_t u1 = raw(cap.u);
    TEST_ASSERT_EQUAL_INT32(0, u1);
}

#ifdef ARDUINO
void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_square_tiling_basic);
    RUN_TEST(test_triangle_tiling_basic);
    RUN_TEST(test_hexagon_tiling_basic);
    RUN_TEST(test_signal_tiling_basic);
    UNITY_END();
}
void loop() {}
#else
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_square_tiling_basic);
    RUN_TEST(test_triangle_tiling_basic);
    RUN_TEST(test_hexagon_tiling_basic);
    RUN_TEST(test_signal_tiling_basic);
    return UNITY_END();
}
#endif
