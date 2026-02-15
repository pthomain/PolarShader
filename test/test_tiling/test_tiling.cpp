//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif
#include <unity.h>
#include "renderer/pipeline/transforms/CartesianTilingTransform.h"

#ifndef ARDUINO
#include "renderer/pipeline/transforms/src/CartesianTilingTransform.cpp"
#endif

using namespace PolarShader;

void test_square_tiling_basic() {
    CartesianTilingTransform transform(64, false, CartesianTilingTransform::TileShape::SQUARE);
    
    UV captured_uv;
    auto mock_layer = [&](UV uv) { 
        captured_uv = uv;
        return PatternNormU16(0); 
    };
    
    UVMap map(mock_layer);
    auto tiled = transform(map);
    
    // UV 0.125 -> sr16 8192. sr8 32.
    // UV 0.125 + 0.25 = 0.375 -> sr16 24576. sr8 96.
    
    tiled(UV(sr16(8192), sr16(8192))); 
    UV uv1 = captured_uv;

    tiled(UV(sr16(24576), sr16(24576))); 
    UV uv2 = captured_uv;
    
    TEST_ASSERT_EQUAL_INT32(raw(uv1.u), raw(uv2.u));
    TEST_ASSERT_EQUAL_INT32(raw(uv1.v), raw(uv2.v));
}

void test_square_tiling_mirrored() {
    CartesianTilingTransform transform(64, true, CartesianTilingTransform::TileShape::SQUARE);
    
    UV captured_uv;
    auto mock_layer = [&](UV uv) { 
        captured_uv = uv;
        return PatternNormU16(0); 
    };
    
    UVMap map(mock_layer);
    auto tiled = transform(map);
    
    // Point in Cell 0: x=32 (sr16 8192)
    tiled(UV(sr16(8192), sr16(8192))); 
    UV p0 = captured_uv;
    TEST_ASSERT_EQUAL_INT32(8192, raw(p0.u));

    // Point in Cell 1: x=96 (sr16 24576)
    tiled(UV(sr16(24576), sr16(8192))); 
    UV p1 = captured_uv;
    
    // to_uv(31) = 31 << 8 = 7936.
    TEST_ASSERT_EQUAL_INT32(7936, raw(p1.u)); 
}

#ifdef ARDUINO
void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_square_tiling_basic);
    RUN_TEST(test_square_tiling_mirrored);
    UNITY_END();
}
void loop() {}
#else
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_square_tiling_basic);
    RUN_TEST(test_square_tiling_mirrored);
    return UNITY_END();
}
#endif
