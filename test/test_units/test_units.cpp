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
#include "renderer/pipeline/maths/NoiseMaths.cpp"
#include "renderer/pipeline/transforms/RotationTransform.cpp"
#include "renderer/pipeline/transforms/ZoomTransform.cpp"
#include "renderer/pipeline/ranges/PolarRange.cpp"
#include "renderer/pipeline/signals/Signals.cpp"
#include "renderer/pipeline/signals/SignalSamplers.cpp"
#include "renderer/pipeline/signals/Accumulators.cpp"
#include "renderer/pipeline/patterns/Patterns.cpp"
#include "renderer/pipeline/patterns/NoisePattern.cpp"
#include "renderer/pipeline/patterns/HexTilingPattern.cpp"
#include "renderer/pipeline/patterns/WorleyPatterns.cpp"
#include "renderer/pipeline/patterns/base/UVPattern.cpp"
#include "renderer/layer/Layer.cpp"
#include "renderer/layer/LayerBuilder.cpp"
#include "renderer/scene/Scene.cpp"
#include "renderer/scene/SceneManager.cpp"
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
    rotation.advanceFrame(FracQ0_16(0), 0);

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
    zoom.advanceFrame(FracQ0_16(0), 0);
    
    UVMap testLayer = [](UV uv) { return PatternNormU16(raw(uv.u)); };
    
    UV input(FracQ16_16(0x0000C000), FracQ16_16(0x00008000));
    PatternNormU16 result = zoom(testLayer)(input);
    
    // With direct-mapped zoom and MIN_SCALE=1/16x (4096):
    // centered x at input.u=0.75 is +0.5 -> fx ~= 0x0800.
    // scaled_uv.u = (0x10000 + 0x0800) >> 1 = 0x8400 (33792).
    TEST_ASSERT_UINT16_WITHIN(500, 33792, raw(result));
}

/** @brief Verify that relative UV signals correctly accumulate over time. */
void test_uv_signal_accumulation() {
    // A constant relative signal returning (0.1, 0.1) per call
    // 0.1 in Q16.16 is ~6554
    UV delta(FracQ16_16(6554), FracQ16_16(6554));
    
    UVSignal rawSignal([delta](FracQ0_16, TimeMillis) {
        return delta;
    }, false); // relative = true
    
    UVSignal resolved = resolveMappedSignal(rawSignal);
    
    // First sample
    UV result1 = resolved(FracQ0_16(100), 0);
    TEST_ASSERT_EQUAL_INT32(6554, raw(result1.u));
    TEST_ASSERT_EQUAL_INT32(6554, raw(result1.v));
    
    // Second sample (should accumulate)
    UV result2 = resolved(FracQ0_16(200), 0);
    TEST_ASSERT_EQUAL_INT32(13108, raw(result2.u));
    TEST_ASSERT_EQUAL_INT32(13108, raw(result2.v));
}

/** @brief Verify PhaseAccumulator integration of signed speed. */
void test_phase_accumulator_signed() {
    // Speed: -0.5 turns per second
    auto speed = MappedSignal<SFracQ0_16>([](FracQ0_16, TimeMillis) {
        return MappedValue(SFracQ0_16(-32768));
    });

    PhaseAccumulator accum(speed);
    accum.advance(FracQ0_16(0), 0); // Init
    
    // Advance 1000ms -> -0.5 turns -> 0.5 (32768)
    FracQ0_16 p1 = accum.advance(FracQ0_16(0), 1000);
    TEST_ASSERT_UINT16_WITHIN(100, 32768, raw(p1));
    
    // Advance another 1000ms -> -1.0 turns -> 0
    FracQ0_16 p2 = accum.advance(FracQ0_16(0), 2000);
    TEST_ASSERT_UINT16_WITHIN(100, 0, raw(p2));
}

/** @brief Verify sine uses speed signal and spans 0..1. */
void test_sine_speed() {
    // Speed: 1.0 turn per second
    SFracQ0_16Signal s = sine(cPerMil(1000));
    
    // t=0 -> 0.5 (32768)
    TEST_ASSERT_INT32_WITHIN(100, 32768, raw(s(FracQ0_16(0), 0)));
    // t=250ms -> 0.25 turn -> s=1.0 -> 65535
    TEST_ASSERT_INT32_WITHIN(100, 65535, raw(s(FracQ0_16(0), 250)));
}

/** @brief Verify zoom driven by sine changes over elapsed time (not treated as constant). */
void test_zoom_transform_sine_varies_over_time() {
    ZoomTransform zoom(sine(cPerMil(1000)));
    UVMap probeLayer = [](UV uv) { return PatternNormU16(raw(uv.u)); };
    UV input(FracQ16_16(0x0000C000), FracQ16_16(0x00008000));

    zoom.advanceFrame(FracQ0_16(0), 0);
    uint16_t a = raw(zoom(probeLayer)(input));

    zoom.advanceFrame(FracQ0_16(0), 250);
    uint16_t b = raw(zoom(probeLayer)(input));

    zoom.advanceFrame(FracQ0_16(0), 500);
    uint16_t c = raw(zoom(probeLayer)(input));

    // If zoom is animated, at least one later sample must differ from the initial one.
    TEST_ASSERT_TRUE(a != b || a != c);
}

/** @brief Verify easing functions loop if period > 0. */
void test_easing_period_looping() {
    // Linear signal looping every 500ms
    SFracQ0_16Signal s = linear(500);
    
    // t=250 -> 0.5 (32767)
    TEST_ASSERT_UINT16_WITHIN(100, 32767, raw(s(FracQ0_16(0), 250)));
    // t=500 -> reset to 0
    TEST_ASSERT_EQUAL_UINT16(0, raw(s(FracQ0_16(0), 500)));
    // t=750 -> 0.5 again
    TEST_ASSERT_UINT16_WITHIN(100, 32767, raw(s(FracQ0_16(0), 750)));
}

/** @brief Verify periodic signals receive scene elapsed time directly. */
void test_periodic_signal_uses_elapsed_time() {
    SFracQ0_16Signal s(
        SignalKind::PERIODIC,
        [](TimeMillis t) {
            return SFracQ0_16(static_cast<int32_t>(t));
        }
    );

    TEST_ASSERT_EQUAL_INT32(42, raw(s(42)));
    TEST_ASSERT_EQUAL_INT32(1234, raw(s(1234)));
}

/** @brief Verify aperiodic RESET mode wraps time by duration modulo. */
void test_aperiodic_reset_wraps_time() {
    SFracQ0_16Signal s(
        SignalKind::APERIODIC,
        LoopMode::RESET,
        1000,
        [](TimeMillis t) {
            return SFracQ0_16(static_cast<int32_t>(t));
        }
    );

    TEST_ASSERT_EQUAL_INT32(250, raw(s(250)));
    TEST_ASSERT_EQUAL_INT32(250, raw(s(1250)));
}

/** @brief Verify aperiodic duration=0 emits zero regardless of waveform. */
void test_aperiodic_zero_duration_emits_zero() {
    SFracQ0_16Signal s(
        SignalKind::APERIODIC,
        LoopMode::RESET,
        0,
        [](TimeMillis) {
            return SFracQ0_16(0xFFFF);
        }
    );

    TEST_ASSERT_EQUAL_INT32(0, raw(s(0)));
    TEST_ASSERT_EQUAL_INT32(0, raw(s(1000)));
}

/** @brief Verify unclamped sampling preserves signed internal values. */
void test_signal_sample_unclamped() {
    SFracQ0_16Signal s(
        SignalKind::PERIODIC,
        [](TimeMillis) {
            return SFracQ0_16(-1000);
        }
    );

    TEST_ASSERT_EQUAL_INT32(-1000, raw(s.sampleUnclamped(123)));
    TEST_ASSERT_EQUAL_INT32(0, raw(s(123)));
}

/** @brief Verify signed negative speed reverses sine direction. */
void test_sine_negative_speed_reverses_direction() {
    SFracQ0_16Signal forward = sine(cPerMil(1000));
    SFracQ0_16Signal reverse = sine(cPerMil(-1000));

    // Prime accumulators (first sample only initializes elapsed baseline).
    (void) forward(0);
    (void) reverse(0);

    // +0.25 turns -> max, -0.25 turns (==0.75) -> min
    TEST_ASSERT_INT32_WITHIN(200, 0xFFFF, raw(forward(250)));
    TEST_ASSERT_INT32_WITHIN(200, 0x0000, raw(reverse(250)));
}

/** @brief Verify mapped-signal resolver is a no-op under the new model. */
void test_mapped_signal_resolver_noop() {
    auto signal = MappedSignal<int32_t>([](FracQ0_16, TimeMillis) {
        return MappedValue<int32_t>(1);
    });

    auto resolved = resolveMappedSignal(signal);
    TEST_ASSERT_EQUAL_INT32(1, resolved(FracQ0_16(0), 0).get());
    TEST_ASSERT_EQUAL_INT32(1, resolved(FracQ0_16(0), 1).get());
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
    RUN_TEST(test_phase_accumulator_signed);
    RUN_TEST(test_sine_speed);
    RUN_TEST(test_zoom_transform_sine_varies_over_time);
    RUN_TEST(test_easing_period_looping);
    RUN_TEST(test_periodic_signal_uses_elapsed_time);
    RUN_TEST(test_aperiodic_reset_wraps_time);
    RUN_TEST(test_aperiodic_zero_duration_emits_zero);
    RUN_TEST(test_signal_sample_unclamped);
    RUN_TEST(test_sine_negative_speed_reverses_direction);
    RUN_TEST(test_mapped_signal_resolver_noop);
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
    RUN_TEST(test_phase_accumulator_signed);
    RUN_TEST(test_sine_speed);
    RUN_TEST(test_zoom_transform_sine_varies_over_time);
    RUN_TEST(test_easing_period_looping);
    RUN_TEST(test_periodic_signal_uses_elapsed_time);
    RUN_TEST(test_aperiodic_reset_wraps_time);
    RUN_TEST(test_aperiodic_zero_duration_emits_zero);
    RUN_TEST(test_signal_sample_unclamped);
    RUN_TEST(test_sine_negative_speed_reverses_direction);
    RUN_TEST(test_mapped_signal_resolver_noop);
    RUN_TEST(test_linear_range);
    RUN_TEST(test_uv_range);
    return UNITY_END();
}
#endif
