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
#include "renderer/pipeline/maths/ZoomMaths.cpp"
#include "renderer/pipeline/maths/PatternMaths.cpp"
#include "renderer/pipeline/transforms/polar/RotationTransform.cpp"
#include "renderer/pipeline/transforms/cartesian/ZoomTransform.cpp"
#include "renderer/pipeline/ranges/PolarRange.cpp"
#include "renderer/pipeline/ranges/ZoomRange.cpp"
#include "renderer/pipeline/ranges/DepthRange.cpp"
#include "renderer/pipeline/ranges/SFracRange.cpp"
#include "renderer/pipeline/ranges/Range.h"
#include "renderer/pipeline/signals/Signals.cpp"
#include "renderer/pipeline/signals/SignalSamplers.cpp"
#include "renderer/pipeline/signals/Accumulators.cpp"
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

#include "renderer/pipeline/transforms/polar/RotationTransform.h"
#include "renderer/pipeline/transforms/cartesian/ZoomTransform.h"

/** @brief Verify that RotationTransform correctly rotates Cartesian UV coordinates. */
void test_rotation_transform_uv() {
    // Rotate 90 degrees (0.25 turns = 0x4000)
    RotationTransform rotation(constant(SFracQ0_16(0x4000)));
    rotation.advanceFrame(0);

    UVLayer testLayer = [](UV uv) {
        return PatternNormU16(raw(uv.u));
    };

    // Input Cartesian UV: (1.0, 0.5) -> Centered (1.0, 0.0)
    // Rotated 90 deg -> Centered (0.0, 1.0)
    // Back to Cartesian UV -> (0.5, 1.0)
    // Expected U: 0.5 (0x8000)
    UV input(FracQ16_16(0x00010000), FracQ16_16(0x00008000));
    
    UVLayer transformedLayer = rotation(testLayer);
    PatternNormU16 result = transformedLayer(input);

    TEST_ASSERT_UINT16_WITHIN(100, 0x8000, raw(result));
}

/** @brief Verify that ZoomTransform correctly scales Cartesian UV coordinates relative to the center. */
void test_zoom_transform_uv() {
    // Use a fixed range of 0.5 (0x8000) to ensure the scale is exactly 0.5
    // regardless of smoothing or signal value.
    ZoomRange range(SFracQ0_16(0x8000), SFracQ0_16(0x8000));
    
    // We need to create a mapped signal manually.
    // Since min=max, the input to mapSignal doesn't matter.
    auto mappedSignal = range.mapSignal(constant(SFracQ0_16(0)));
    
    // Use the constructor that accepts the mapped signal and the range
    // But wait, that constructor expects MappedSignal<SFracQ0_16>, which mapSignal returns.
    // However, ZoomTransform constructor is:
    // explicit ZoomTransform(MappedSignal<SFracQ0_16> scale, ZoomRange range);
    // Note: ZoomRange is passed by value (copy/move).
    
    // Wait, that constructor is private in the header I read earlier?
    // Let's check ZoomTransform.h again.
    // It says: explicit ZoomTransform(MappedSignal<SFracQ0_16> scale, ZoomRange range); is PRIVATE.
    // Only public is: explicit ZoomTransform(SFracQ0_16Signal scale);
    
    // If I can't set the range publicly, I have to rely on default range.
    // Default range min is ~0.00625.
    // Signal 0 -> min.
    // If I use constant(SFracQ0_16(0)), target is min.
    // ScaleValue starts at min.
    // So if I set signal to 0, scale should stay at min.
    // min = scale32(Q0_16_MAX, frac(160)) ~ 409.
    
    // That's too small to test easily.
    // If I set signal to 1.0 (max), target is 4.0.
    // It will smooth towards 4.0.
    
    // I need a way to set immediate scale or use a known start point.
    // ZoomTransform state initializes scaleValue = minScaleRaw.
    // So if I know minScaleRaw, I know the starting scale.
    // MIN_SCALE = scale32(65535, frac(160)).
    // frac(160) = 65535 / 160 = 409.
    // scale32(65535, 409) = 409.
    
    // So start scale is 409 (0x0199).
    // Let's verify this behavior.
    
    // If I use constant(SFracQ0_16(0)), target is min (409).
    // Current is min (409).
    // Delta 0.
    // Scale stays 409.
    
    // Test:
    // Input (0.75, 0.5) -> Centered (0.5, 0.0).
    // Scale 409/65536 ~= 0.00624.
    // Scaled x = 0.5 * 0.00624 = 0.00312.
    // Back to UV: (0.00312 + 1)/2 = 0.50156.
    // 0.50156 * 65536 = 32870 (0x8066).
    
    ZoomTransform zoom(constant(SFracQ0_16(0))); // Target min
    zoom.advanceFrame(0);
    
    UVLayer testLayer = [](UV uv) { return PatternNormU16(raw(uv.u)); };
    
    UV input(FracQ16_16(0x0000C000), FracQ16_16(0x00008000));
    PatternNormU16 result = zoom(testLayer)(input);
    
    // We expect it to be very close to 0.5 (0x8000) because of extreme zoom out.
    // 409 is the raw scale.
    // 0.5 (Q16.16: 32768) * 409 = 13402112. >> 16 = 204.
    // absolute result = (204 + 65536)/2 = 65740 / 2 = 32870.
    
    TEST_ASSERT_UINT16_WITHIN(10, 32870, raw(result));
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
    return UNITY_END();
}
#endif
