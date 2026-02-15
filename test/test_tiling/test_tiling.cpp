//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif
#include <unity.h>
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/transforms/CartesianTilingTransform.h"

#ifndef ARDUINO
#include "renderer/pipeline/signals/src/Signals.cpp"
#include "renderer/pipeline/signals/src/SignalSamplers.cpp"
#include "renderer/pipeline/signals/src/accumulators/Accumulators.cpp"
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
    
    tiled(UV(sr16(0), sr16(0))); 
    TEST_ASSERT_EQUAL_INT32(0, raw(captured_uv.u));
    TEST_ASSERT_EQUAL_INT32(0, raw(captured_uv.v));
}

void test_triangle_tiling_basic() {
    CartesianTilingTransform transform(64, false, CartesianTilingTransform::TileShape::TRIANGLE);
    
    UV cap;
    auto mock = [&](UV uv) { cap = uv; return PatternNormU16(0); };
    auto tiled = transform(mock);

    tiled(UV(sr16(0), sr16(0)));
    TEST_ASSERT_EQUAL_INT32(0, raw(cap.u));
    TEST_ASSERT_EQUAL_INT32(0, raw(cap.v));
}

void test_hexagon_tiling_basic() {
    CartesianTilingTransform transform(64, false, CartesianTilingTransform::TileShape::HEXAGON);
    
    UV cap;
    auto mock = [&](UV uv) { cap = uv; return PatternNormU16(0); };
    auto tiled = transform(mock);
    
    tiled(UV(sr16(0), sr16(0))); 
    TEST_ASSERT_EQUAL_INT32(0, raw(cap.u));
    TEST_ASSERT_EQUAL_INT32(0, raw(cap.v));
}

void test_signal_tiling_basic() {
    CartesianTilingTransform transform(constant(128)); 
    
    UV cap;
    auto mock = [&](UV uv) { cap = uv; return PatternNormU16(0); };
    auto tiled = transform(mock);
    
    tiled(UV(sr16(0), sr16(0)));
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
