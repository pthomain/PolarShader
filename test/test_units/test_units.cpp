//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#include "native/FastLED.h"
#endif
#include <unity.h>
#include "renderer/pipeline/units/ScalarUnits.h"
#include "renderer/pipeline/units/UVUnits.h"
#include "renderer/pipeline/maths/UVMaths.h"
#include "renderer/pipeline/maths/CartesianMaths.h"
#include "renderer/pipeline/maths/PolarMaths.h"

#ifndef ARDUINO
#include "renderer/pipeline/maths/PolarMaths.cpp"
#include "renderer/pipeline/maths/AngleMaths.cpp"
#include "renderer/pipeline/maths/ScalarMaths.cpp"
#endif

using namespace PolarShader;

void test_frac_q16_16_raw_values() {
    FracQ16_16 f(0x00010000); // 1.0
    TEST_ASSERT_EQUAL_INT32(0x00010000, f.raw());
}

void test_uv_coordinate_structure() {
    UV uv(FracQ16_16(0x00010000), FracQ16_16(0x00008000));
    TEST_ASSERT_EQUAL_INT32(0x00010000, uv.u.raw());
    TEST_ASSERT_EQUAL_INT32(0x00008000, uv.v.raw());
}

void test_uv_addition() {
    UV uv1(FracQ16_16(0x00010000), FracQ16_16(0x00008000));
    UV uv2(FracQ16_16(0x00004000), FracQ16_16(0x00002000));
    UV result = UVMaths::add(uv1, uv2);
    TEST_ASSERT_EQUAL_INT32(0x00014000, result.u.raw());
    TEST_ASSERT_EQUAL_INT32(0x0000A000, result.v.raw());
}

void test_cartesian_uv_conversion() {
    FracQ16_16 uv_val(0x00008000); // 0.5
    CartQ24_8 cart = CartesianMaths::from_uv(uv_val);
    TEST_ASSERT_EQUAL_INT32(0x00000080, cart.raw()); // 0.5 * 256 = 128 (0x80)
    
    FracQ16_16 back = CartesianMaths::to_uv(cart);
    TEST_ASSERT_EQUAL_INT32(0x00008000, back.raw());
}

void test_polar_uv_conversion_center() {
    // Center of screen (0.5, 0.5) should have radius 0
    UV cart_uv(FracQ16_16(0x00008000), FracQ16_16(0x00008000));
    UV polar_uv = cartesianToPolarUV(cart_uv);
    TEST_ASSERT_EQUAL_INT32(0, polar_uv.v.raw()); // Radius should be 0
}

void test_polar_uv_conversion_right() {
    // Right middle (1.0, 0.5) should have angle 0 and radius 1.0 (0xFFFF in Q0.16)
    UV cart_uv(FracQ16_16(0x00010000), FracQ16_16(0x00008000));
    UV polar_uv = cartesianToPolarUV(cart_uv);
    TEST_ASSERT_EQUAL_INT32(0, polar_uv.u.raw()); // Angle 0
    TEST_ASSERT_UINT32_WITHIN(10, 0x0000FFFF, polar_uv.v.raw()); // Radius ~1.0
}

void test_uv_round_trip() {
    // Start with a point, convert to polar and back to cartesian
    // (0.75, 0.75)
    UV original(FracQ16_16(0x0000C000), FracQ16_16(0x0000C000));
    UV polar = cartesianToPolarUV(original);
    UV back = polarToCartesianUV(polar);
    
    TEST_ASSERT_INT32_WITHIN(100, original.u.raw(), back.u.raw());
    TEST_ASSERT_INT32_WITHIN(100, original.v.raw(), back.v.raw());
}

#ifdef ARDUINO
void setup() {
    delay(2000); 
    UNITY_BEGIN();
    RUN_TEST(test_frac_q16_16_raw_values);
    RUN_TEST(test_uv_coordinate_structure);
    RUN_TEST(test_uv_addition);
    RUN_TEST(test_cartesian_uv_conversion);
    RUN_TEST(test_polar_uv_conversion_center);
    RUN_TEST(test_polar_uv_conversion_right);
    RUN_TEST(test_uv_round_trip);
    UNITY_END();
}

void loop() {}
#else
int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_frac_q16_16_raw_values);
    RUN_TEST(test_uv_coordinate_structure);
    RUN_TEST(test_uv_addition);
    RUN_TEST(test_cartesian_uv_conversion);
    RUN_TEST(test_polar_uv_conversion_center);
    RUN_TEST(test_polar_uv_conversion_right);
    RUN_TEST(test_uv_round_trip);
    return UNITY_END();
}
#endif
