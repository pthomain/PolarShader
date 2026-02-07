//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#include "native/FastLED.h"
#endif
#include <unity.h>
#include "renderer/pipeline/maths/units/Units.h"
#include "renderer/pipeline/maths/UVMaths.h"
#include "renderer/pipeline/maths/CartesianMaths.h"
#include "renderer/pipeline/maths/PolarMaths.h"

#ifndef ARDUINO
#include "renderer/pipeline/maths/PolarMaths.cpp"
#include "renderer/pipeline/transforms/RotationTransform.cpp"
#include "renderer/pipeline/transforms/ZoomTransform.cpp"
#include "renderer/pipeline/ranges/PolarRange.cpp"
#include "renderer/pipeline/signals/Signals.cpp"
#include "renderer/pipeline/signals/SignalSamplers.cpp"
#include "renderer/pipeline/signals/Accumulators.cpp"
#include "renderer/pipeline/Layer.cpp"
#include "renderer/pipeline/LayerBuilder.cpp"
#include "renderer/pipeline/Scene.cpp"
#include "renderer/pipeline/SceneManager.cpp"
#endif

using namespace PolarShader;

/** @brief Verify that FracQ16_16 correctly stores 32-bit fixed-point values. */
void test_frac_q16_16_raw_values() {
    FracQ16_16 f(0x00010000); // 1.0
    TEST_ASSERT_EQUAL_INT32(0x00010000, f.raw());
}

/** @brief Verify that the UV structure correctly holds U and V components. */
void test_uv_coordinate_structure() {
    UV uv(FracQ16_16(0x00010000), FracQ16_16(0x00008000));
    TEST_ASSERT_EQUAL_INT32(0x00010000, uv.u.raw());
    TEST_ASSERT_EQUAL_INT32(0x00008000, uv.v.raw());
}

/** @brief Verify basic additive arithmetic for UV coordinates. */
void test_uv_addition() {
    UV uv1(FracQ16_16(0x00010000), FracQ16_16(0x00008000));
    UV uv2(FracQ16_16(0x00004000), FracQ16_16(0x00002000));
    UV result = UVMaths::add(uv1, uv2);
    TEST_ASSERT_EQUAL_INT32(0x00014000, result.u.raw());
    TEST_ASSERT_EQUAL_INT32(0x0000A000, result.v.raw());
}

/** @brief Verify conversion between UV (Q16.16) and legacy CartQ24.8. */
void test_cartesian_uv_conversion() {
    FracQ16_16 uv_val(0x00008000); // 0.5
    CartQ24_8 cart = CartesianMaths::from_uv(uv_val);
    TEST_ASSERT_EQUAL_INT32(0x00000080, cart.raw()); // 0.5 * 256 = 128 (0x80)
    
    FracQ16_16 back = CartesianMaths::to_uv(cart);
    TEST_ASSERT_EQUAL_INT32(0x00008000, back.raw());
}

/** @brief Verify that the center of the UV domain maps to the origin of the Polar domain. */
void test_polar_uv_conversion_center() {
    // Center of screen (0.5, 0.5) should have radius 0
    UV cart_uv(FracQ16_16(0x00008000), FracQ16_16(0x00008000));
    UV polar_uv = cartesianToPolarUV(cart_uv);
    TEST_ASSERT_EQUAL_INT32(0, polar_uv.v.raw()); // Radius should be 0
}

/** @brief Verify that points on the right edge map to angle 0 and radius 1.0. */
void test_polar_uv_conversion_right() {
    // Right middle (1.0, 0.5) should have angle 0 and radius 1.0 (0xFFFF in Q0.16)
    UV cart_uv(FracQ16_16(0x00010000), FracQ16_16(0x00008000));
    UV polar_uv = cartesianToPolarUV(cart_uv);
    TEST_ASSERT_EQUAL_INT32(0, polar_uv.u.raw()); // Angle 0
    TEST_ASSERT_UINT32_WITHIN(10, 0x0000FFFF, polar_uv.v.raw()); // Radius ~1.0
}

/** @brief Verify that a full conversion round-trip preserves precision. */
void test_uv_round_trip() {
    // Start with a point, convert to polar and back to cartesian
    // (0.75, 0.75)
    UV original(FracQ16_16(0x0000C000), FracQ16_16(0x0000C000));
    UV polar = cartesianToPolarUV(original);
    UV back = polarToCartesianUV(polar);
    
    TEST_ASSERT_INT32_WITHIN(100, original.u.raw(), back.u.raw());
    TEST_ASSERT_INT32_WITHIN(100, original.v.raw(), back.v.raw());
}

#include "renderer/pipeline/transforms/RotationTransform.h"
#include "renderer/pipeline/transforms/ZoomTransform.h"

/** @brief Verify that RotationTransform correctly rotates Cartesian UV coordinates. */
void test_rotation_transform_uv() {
    // Rotate 90 degrees (0.25 turns = 0x4000)
    RotationTransform rotation(constant(SFracQ0_16(0x4000)));
    rotation.advanceFrame(0);

    UVMap testLayer = [](UV uv) {
        return PatternNormU16(raw(uv.u));
    };

    // Input Cartesian UV: (1.0, 0.5) -> Centered (1.0, 0.0)
    // Rotated 90 deg -> Centered (0.0, 1.0)
    // Back to Cartesian UV -> (0.5, 1.0)
    // Expected U: 0.5 (0x8000)
    UV input(FracQ16_16(0x00010000), FracQ16_16(0x00008000));
    
    UVMap transformedLayer = rotation(testLayer);
    PatternNormU16 result = transformedLayer(input);

    TEST_ASSERT_UINT16_WITHIN(100, 0x8000, raw(result));
}

/** @brief Verify that ZoomTransform correctly scales Cartesian UV coordinates relative to the center. */
void test_zoom_transform_uv() {
    ZoomTransform zoom(constant(SFracQ0_16(0))); // Target min
    zoom.advanceFrame(0);
    
    UVMap testLayer = [](UV uv) { return PatternNormU16(raw(uv.u)); };
    
    UV input(FracQ16_16(0x0000C000), FracQ16_16(0x00008000));
    PatternNormU16 result = zoom(testLayer)(input);
    
    // With current defaults: MIN_SCALE=1/16x (4096), Initial scaleValue=0.
    // ScaleValue 0 means sx = 0, sy = 0.
    // fx = 0, fy = 0.
    // scaled_uv = (0.5, 0.5).
    // Pattern result at (0.5, 0.5) is input.u = 0.5 (for centered coords).
    // WAIT, let's re-calculate:
    // UV scaled_uv(
    //    FracQ16_16((fx + 0x00010000) >> 1),
    //    FracQ16_16((fy + 0x00010000) >> 1)
    // );
    // fx=0 => scaled_uv.u = 0x10000 >> 1 = 0x8000 (0.5).
    // 0.5 * 65536 = 32768.
    
    TEST_ASSERT_UINT16_WITHIN(100, 32768, raw(result));
}

/** @brief Verify that relative UV signals correctly accumulate over time. */
void test_uv_signal_accumulation() {
    // A constant relative signal returning (0.1, 0.1) per call
    // 0.1 in Q16.16 is ~6554
    UV delta(FracQ16_16(6554), FracQ16_16(6554));
    
    UVSignal rawSignal([delta](TimeMillis) {
        return MappedValue<UV>(delta);
    }, false); // relative = true
    
    UVSignal resolved = resolveMappedSignal(rawSignal);
    
    // First sample
    UV result1 = resolved(100).get();
    TEST_ASSERT_EQUAL_INT32(6554, raw(result1.u));
    TEST_ASSERT_EQUAL_INT32(6554, raw(result1.v));
    
    // Second sample (should accumulate)
    UV result2 = resolved(200).get();
    TEST_ASSERT_EQUAL_INT32(13108, raw(result2.u));
    TEST_ASSERT_EQUAL_INT32(13108, raw(result2.v));
}

#include "renderer/pipeline/ranges/LinearRange.h"
#include "renderer/pipeline/ranges/UVRange.h"

/** @brief Verify that LinearRange correctly interpolates values. */
void test_linear_range() {
    LinearRange<SFracQ0_16> range(SFracQ0_16(0), SFracQ0_16(1000));
    
    // 0.0 -> 0
    TEST_ASSERT_EQUAL_INT32(0, raw(range.map(SFracQ0_16(0)).get()));
    // 0.5 -> 500
    TEST_ASSERT_EQUAL_INT32(500, raw(range.map(SFracQ0_16(0x8000)).get()));
    // 1.0 -> 1000
    TEST_ASSERT_EQUAL_INT32(1000, raw(range.map(SFracQ0_16(0xFFFF)).get()));
}

/** @brief Verify that UVRange correctly interpolates 2D coordinates. */
void test_uv_range() {
    UV min(FracQ16_16(0), FracQ16_16(0));
    UV max(FracQ16_16(0x10000), FracQ16_16(0x10000));
    UVRange range(min, max);
    
    // 0.5 -> (0.5, 0.5)
    UV result = range.map(SFracQ0_16(0x8000)).get();
    TEST_ASSERT_EQUAL_INT32(0x8000, raw(result.u));
    TEST_ASSERT_EQUAL_INT32(0x8000, raw(result.v));
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
    RUN_TEST(test_rotation_transform_uv);
    RUN_TEST(test_zoom_transform_uv);
    RUN_TEST(test_uv_signal_accumulation);
    RUN_TEST(test_linear_range);
    RUN_TEST(test_uv_range);
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
    RUN_TEST(test_rotation_transform_uv);
    RUN_TEST(test_zoom_transform_uv);
    RUN_TEST(test_uv_signal_accumulation);
    RUN_TEST(test_linear_range);
    RUN_TEST(test_uv_range);
    return UNITY_END();
}
#endif