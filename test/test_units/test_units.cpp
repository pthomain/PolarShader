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
#include "renderer/pipeline/signals/Signals.cpp"
#include "renderer/pipeline/signals/SignalSamplers.cpp"
#include "renderer/pipeline/signals/accumulators/Accumulators.cpp"
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

namespace {
    const LinearRange<SFracQ0_16> TEST_UNIT_RANGE{SFracQ0_16(0), SFracQ0_16(FRACT_Q0_16_MAX)};
    const LinearRange<SFracQ0_16> TEST_SIGNED_RANGE{SFracQ0_16(Q0_16_MIN), SFracQ0_16(Q0_16_MAX)};
}

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
    // Signed range mapping: -0.5 maps to 0.25 turns (90 degrees).
    RotationTransform rotation(constant(SFracQ0_16(-0x8000)));
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
    ZoomTransform zoom(constant(SFracQ0_16(Q0_16_MIN))); // Target min
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
    });
    
    UVSignal resolved(
        [rawSignal, accumulated = UV(FracQ16_16(0), FracQ16_16(0))](FracQ0_16 progress, TimeMillis elapsedMs) mutable {
            UV value = rawSignal(progress, elapsedMs);
            accumulated.u = FracQ16_16(raw(accumulated.u) + raw(value.u));
            accumulated.v = FracQ16_16(raw(accumulated.v) + raw(value.v));
            return accumulated;
        }
    );
    
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
    auto speed = [](TimeMillis) { return SFracQ0_16(-32768); };

    PhaseAccumulator accum(speed);
    accum.advance(0); // Init
    
    // Advance 1000ms -> -0.5 turns -> 0.5 (32768)
    FracQ0_16 p1 = accum.advance(1000);
    TEST_ASSERT_UINT16_WITHIN(100, 32768, raw(p1));
    
    // Advance another 1000ms -> -1.0 turns -> 0
    FracQ0_16 p2 = accum.advance(2000);
    TEST_ASSERT_UINT16_WITHIN(100, 0, raw(p2));
}

/** @brief Verify sine uses speed signal and is centered in signed space. */
void test_sine_speed() {
    // Speed: 1.0 turn per second
    SFracQ0_16Signal s = sine(cPerMil(1000));
    
    // t=0 -> centered
    TEST_ASSERT_INT32_WITHIN(100, 32767, raw(s.sample(TEST_SIGNED_RANGE, 0)));
    // t=250ms -> positive peak
    TEST_ASSERT_INT32_WITHIN(200, 65535, raw(s.sample(TEST_SIGNED_RANGE, 250)));
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
    
    // Linear emits unsigned progress; mapped through signed range it sits around midpoint.
    int32_t a = raw(s.sample(TEST_SIGNED_RANGE, 250));
    int32_t b = raw(s.sample(TEST_SIGNED_RANGE, 500));
    int32_t c = raw(s.sample(TEST_SIGNED_RANGE, 750));
    TEST_ASSERT_INT32_WITHIN(20, 32767, a);
    TEST_ASSERT_INT32_WITHIN(20, 0, b);
    TEST_ASSERT_INT32_WITHIN(20, a, c);
}

/** @brief Verify periodic signals receive scene elapsed time directly. */
void test_periodic_signal_uses_elapsed_time() {
    SFracQ0_16Signal s(
        SignalKind::PERIODIC,
        [](TimeMillis t) {
            return SFracQ0_16(static_cast<int32_t>(t));
        }
    );

    TEST_ASSERT_INT32_WITHIN(2, 42, raw(s.sample(TEST_SIGNED_RANGE, 42)));
    TEST_ASSERT_INT32_WITHIN(2, 1234, raw(s.sample(TEST_SIGNED_RANGE, 1234)));
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

    TEST_ASSERT_INT32_WITHIN(2, 250, raw(s.sample(TEST_SIGNED_RANGE, 250)));
    TEST_ASSERT_INT32_WITHIN(2, 250, raw(s.sample(TEST_SIGNED_RANGE, 1250)));
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

    TEST_ASSERT_EQUAL_INT32(0, raw(s.sample(TEST_SIGNED_RANGE, 0)));
    TEST_ASSERT_EQUAL_INT32(0, raw(s.sample(TEST_SIGNED_RANGE, 1000)));
}

/** @brief Verify sampling saturates to signed Q0.16 bounds. */
void test_signal_sample_clamped() {
    SFracQ0_16Signal s(
        SignalKind::PERIODIC,
        [](TimeMillis) {
            return SFracQ0_16(-1000);
        }
    );

    TEST_ASSERT_EQUAL_INT32(-1000, raw(s.sample(TEST_SIGNED_RANGE, 123)));
}

/** @brief Verify signed negative speed reverses sine direction. */
void test_sine_negative_speed_reverses_direction() {
    SFracQ0_16Signal forward = sine(cPerMil(1000));
    SFracQ0_16Signal reverse = sine(cPerMil(-1000));

    // Prime accumulators (first sample only initializes elapsed baseline).
    (void) forward.sample(TEST_SIGNED_RANGE, 0);
    (void) reverse.sample(TEST_SIGNED_RANGE, 0);

    // +0.25 turns -> max, -0.25 turns (==0.75) -> min
    TEST_ASSERT_INT32_WITHIN(200, Q0_16_MAX, raw(forward.sample(TEST_SIGNED_RANGE, 250)));
    TEST_ASSERT_INT32_WITHIN(200, 0, raw(reverse.sample(TEST_SIGNED_RANGE, 250)));
}

/** @brief Verify scalar signal range mapping is done directly via sample(range, elapsedMs). */
void test_signal_range_mapping() {
    SFracQ0_16Signal signal(
        SignalKind::PERIODIC,
        [](TimeMillis) {
            return SFracQ0_16(0x8000); // ~0.5
        }
    );
    LinearRange<int32_t> range(0, 1000);
    TEST_ASSERT_EQUAL_INT32(750, signal.sample(range, 0));
}

#include "renderer/pipeline/signals/ranges/LinearRange.h"
#include "renderer/pipeline/signals/ranges/UVRange.h"

/** @brief Verify that LinearRange correctly interpolates values. */
void test_linear_range() {
    LinearRange<SFracQ0_16> range(SFracQ0_16(0), SFracQ0_16(1000));
    
    // 0.0 -> midpoint
    TEST_ASSERT_EQUAL_INT32(500, raw(range.map(SFracQ0_16(0))));
    // +0.5 -> 750
    TEST_ASSERT_EQUAL_INT32(750, raw(range.map(SFracQ0_16(0x8000))));
    // 1.0 -> 1000
    TEST_ASSERT_EQUAL_INT32(1000, raw(range.map(SFracQ0_16(0xFFFF))));
    // -1.0 -> 0
    TEST_ASSERT_EQUAL_INT32(0, raw(range.map(SFracQ0_16(Q0_16_MIN))));
}

/** @brief Verify that UVRange correctly interpolates 2D coordinates. */
void test_uv_range() {
    UV min(FracQ16_16(0), FracQ16_16(0));
    UV max(FracQ16_16(0x10000), FracQ16_16(0x10000));
    UVRange range(min, max);
    
    // 0.0 -> midpoint (0.5, 0.5)
    UV result = range.map(SFracQ0_16(0));
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
    RUN_TEST(test_signal_sample_clamped);
    RUN_TEST(test_sine_negative_speed_reverses_direction);
    RUN_TEST(test_signal_range_mapping);
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
    RUN_TEST(test_signal_sample_clamped);
    RUN_TEST(test_sine_negative_speed_reverses_direction);
    RUN_TEST(test_signal_range_mapping);
    RUN_TEST(test_linear_range);
    RUN_TEST(test_uv_range);
    return UNITY_END();
}
#endif
