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

void test_triangle_tiling_basic() {
    CartesianTilingTransform transform(64, false, CartesianTilingTransform::TileShape::TRIANGLE);
    
    UV captured_uv;
    auto mock_layer = [&](UV uv) { 
        captured_uv = uv;
        return PatternNormU16(0); 
    };
    
    UVMap map(mock_layer);
    auto tiled = transform(map);
    
    // (16, 16) and (48, 16) in sr8. 
    // In Square: Both have local_x = 16 or 48? No, col=0 for both if width=64.
    // Wait, (16,16) -> col=0, local_x=16.
    // (48,16) -> col=0, local_x=48.
    
    // In Triangle tiling with side 64:
    // (16, 16) is in an "Up" triangle.
    // (48, 16) is in a "Down" triangle.
    
    // They should NOT have the same local coordinates relative to a square bounding box.
    // Actually, let's just assert something that is definitely different.
    
    tiled(UV(sr16(4096), sr16(4096))); // sr8: (16, 16)
    UV uv_up = captured_uv;

    tiled(UV(sr16(12288), sr16(4096))); // sr8: (48, 16)
    UV uv_down = captured_uv;

    // In square tiling, both would have v = 4096.
    // In triangle tiling, the "Down" triangle is inverted, so its local v should be flipped if we normalize.
    // But we use CONSISTENT SCALE.
    
    // Let's check that the horizontal distance between them in local space is NOT 32.
    // In square tiling: 48 - 16 = 32.
    // In triangle tiling: They are in DIFFERENT triangles.
    
    TEST_ASSERT_NOT_EQUAL_INT32(32 << 8, raw(uv_down.u) - raw(uv_up.u));
}

void test_triangle_tiling_symmetry() {
    // We check that points that should be equivalent in an equilateral triangle are mapped correctly.
    // Center is (0,0) in local space because we subtract halfSide/2 and h/2.
    // Wait, halfSide/2 is cellSizeRaw/4. 
    
    CartesianTilingTransform transform(64, false, CartesianTilingTransform::TileShape::TRIANGLE);
    
    UV captured_uv;
    auto mock_layer = [&](UV uv) { 
        captured_uv = uv;
        return PatternNormU16(0); 
    };
    
    UVMap map(mock_layer);
    auto tiled = transform(map);
    
    // Pick a point in the first "Up" triangle.
    // x = 16, y = 16 (in sr8).
    // In our implementation: 
    // side = 64, halfSide = 32, h = 55.
    // col = 0, row = 0. is_even = true.
    // rem_x = 16, rem_y = 16.
    // above_diagonal = (16 * 32) < ((32 - 16) * 55) => 512 < 16 * 55 (880). True.
    // is_up = true.
    // local_x = 16 - 16 = 0.
    // local_y = 16 - 27 = -11.
    
    tiled(UV(sr16(4096), sr16(4096))); 
    UV uv1 = captured_uv;
    
    // Now pick a point in another "Up" triangle.
    // (x + side, y) = (80, 16)
    // col = 80 / 32 = 2. row = 0.
    // rem_x = 80 - 64 = 16. rem_y = 16.
    // is_even = true.
    // is_up = true.
    
    tiled(UV(sr16(20480), sr16(4096))); 
    UV uv2 = captured_uv;
    
    TEST_ASSERT_EQUAL_INT32(raw(uv1.u), raw(uv2.u));
    TEST_ASSERT_EQUAL_INT32(raw(uv1.v), raw(uv2.v));
}

void test_hexagon_tiling_basic() {
    // Side length 64.
    CartesianTilingTransform transform(64, false, CartesianTilingTransform::TileShape::HEXAGON);
    
    UV captured_uv;
    auto mock_layer = [&](UV uv) { 
        captured_uv = uv;
        return PatternNormU16(0); 
    };
    
    UVMap map(mock_layer);
    auto tiled = transform(map);
    
    // For pointy-top hexagon with side s=64:
    // Width = s * sqrt(3) = 64 * 1.732 = 110.8 -> 111 in sr8.
    // Horizontal spacing between hex centers = Width = 111.
    
    tiled(UV(sr16(0), sr16(0))); 
    UV uv1 = captured_uv;

    // Shift by Width in sr8: Width << 8 = 111 << 8 = 28416.
    // We use the exact calculation our implementation will use: (64 * 443) >> 8 = 110.75 -> 110 or 111.
    // sqrt(3) in Q8.8 is 443.
    int32_t w = (64 * 443) >> 8;
    
    tiled(UV(sr16(w << 8), sr16(0))); 
    UV uv2 = captured_uv;
    
    // They should be in the same relative position in different hexes.
    TEST_ASSERT_EQUAL_INT32(raw(uv1.u), raw(uv2.u));
    TEST_ASSERT_EQUAL_INT32(raw(uv1.v), raw(uv2.v));
}

#ifdef ARDUINO
void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_square_tiling_basic);
    RUN_TEST(test_square_tiling_mirrored);
    RUN_TEST(test_triangle_tiling_basic);
    RUN_TEST(test_triangle_tiling_symmetry);
    RUN_TEST(test_hexagon_tiling_basic);
    UNITY_END();
}
void loop() {}
#else
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_square_tiling_basic);
    RUN_TEST(test_square_tiling_mirrored);
    RUN_TEST(test_triangle_tiling_basic);
    RUN_TEST(test_triangle_tiling_symmetry);
    RUN_TEST(test_hexagon_tiling_basic);
    return UNITY_END();
}
#endif
