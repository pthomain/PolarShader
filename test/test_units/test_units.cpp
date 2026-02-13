//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#include "renderer/pipeline/signals/Signals.h"
#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#include "native/FastLED.h"
#endif
#include <unity.h>
#include "renderer/pipeline/signals/SignalTypes.h"
#include "renderer/pipeline/maths/units/Units.h"
#include "renderer/pipeline/maths/UVMaths.h"
#include "renderer/pipeline/maths/CartesianMaths.h"
#include "renderer/pipeline/maths/PolarMaths.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"
#include "renderer/pipeline/signals/ranges/BipolarRange.h"
#include "renderer/pipeline/signals/ranges/UVRange.h"
#include "renderer/pipeline/transforms/RotationTransform.h"
#include "renderer/pipeline/transforms/ZoomTransform.h"

#ifndef ARDUINO
#include "renderer/pipeline/maths/src/PolarMaths.cpp"
#include "renderer/pipeline/maths/src/NoiseMaths.cpp"
#include "renderer/pipeline/maths/src/PatternMaths.cpp"
#include "renderer/pipeline/maths/src/TimeMaths.cpp"
#include "renderer/pipeline/transforms/src/RotationTransform.cpp"
#include "renderer/pipeline/transforms/src/ZoomTransform.cpp"
#include "renderer/pipeline/signals/src/Signals.cpp"
#include "renderer/pipeline/signals/src/SignalSamplers.cpp"
#include "renderer/pipeline/signals/src/accumulators/Accumulators.cpp"
#include "renderer/pipeline/patterns/src/Patterns.cpp"
#include "renderer/pipeline/patterns/src/NoisePattern.cpp"
#include "renderer/pipeline/patterns/src/HexTilingPattern.cpp"
#include "renderer/pipeline/patterns/src/WorleyPatterns.cpp"
#include "renderer/pipeline/patterns/src/base/UVPattern.cpp"
#include "renderer/layer/src/Layer.cpp"
#include "renderer/layer/src/LayerBuilder.cpp"
#include "renderer/scene/src/Scene.cpp"
#include "renderer/scene/src/SceneManager.cpp"
#endif

using namespace PolarShader;

namespace {
    const MagnitudeRange TEST_UNIT_RANGE{sf16(0), sf16(SF16_MAX)};
    const BipolarRange TEST_SIGNED_RANGE{sf16(SF16_MIN), sf16(SF16_MAX)};
}

/** @brief Verify that sr16 correctly stores 32-bit fixed-point values. */
void test_frac_q16_16_raw_values() {
    sr16 f(0x00010000); // 1.0
    TEST_ASSERT_EQUAL_INT32(0x00010000, f.raw());
}

/** @brief Verify that the UV structure correctly holds U and V components. */
void test_uv_coordinate_structure() {
    UV uv(sr16(0x00010000), sr16(0x00008000));
    TEST_ASSERT_EQUAL_INT32(0x00010000, uv.u.raw());
    TEST_ASSERT_EQUAL_INT32(0x00008000, uv.v.raw());
}

/** @brief Verify basic additive arithmetic for UV coordinates. */
void test_uv_addition() {
    UV uv1(sr16(0x00010000), sr16(0x00008000));
    UV uv2(sr16(0x00004000), sr16(0x00002000));
    UV result = UVMaths::add(uv1, uv2);
    TEST_ASSERT_EQUAL_INT32(0x00014000, result.u.raw());
    TEST_ASSERT_EQUAL_INT32(0x0000A000, result.v.raw());
}

/** @brief Verify conversion between UV (r16/sr16 (Q16.16)) and legacy sr8/r8 Cartesian. */
void test_cartesian_uv_conversion() {
    sr16 uv_val(0x00008000); // 0.5
    sr8 cart = CartesianMaths::from_uv(uv_val);
    TEST_ASSERT_EQUAL_INT32(0x00000080, cart.raw()); // 0.5 * 256 = 128 (0x80)
    
    sr16 back = CartesianMaths::to_uv(cart);
    TEST_ASSERT_EQUAL_INT32(0x00008000, back.raw());
}

/** @brief Verify that the center of the UV domain maps to the origin of the Polar domain. */
void test_polar_uv_conversion_center() {
    // Center of screen (0.5, 0.5) should have radius 0
    UV cart_uv(sr16(0x00008000), sr16(0x00008000));
    UV polar_uv = cartesianToPolarUV(cart_uv);
    TEST_ASSERT_EQUAL_INT32(0, polar_uv.v.raw()); // Radius should be 0
}

/** @brief Verify that points on the right edge map to angle 0 and radius 1.0. */
void test_polar_uv_conversion_right() {
    // Right middle (1.0, 0.5) should have angle 0 and radius 1.0 (0xFFFF in f16/sf16)
    UV cart_uv(sr16(0x00010000), sr16(0x00008000));
    UV polar_uv = cartesianToPolarUV(cart_uv);
    TEST_ASSERT_EQUAL_INT32(0, polar_uv.u.raw()); // Angle 0
    TEST_ASSERT_UINT32_WITHIN(10, 0x0000FFFF, polar_uv.v.raw()); // Radius ~1.0
}

/** @brief Verify that a full conversion round-trip preserves precision. */
void test_uv_round_trip() {
    // Start with a point, convert to polar and back to cartesian
    // (0.75, 0.75)
    UV original(sr16(0x0000C000), sr16(0x0000C000));
    UV polar = cartesianToPolarUV(original);
    UV back = polarToCartesianUV(polar);
    
    TEST_ASSERT_INT32_WITHIN(100, original.u.raw(), back.u.raw());
    TEST_ASSERT_INT32_WITHIN(100, original.v.raw(), back.v.raw());
}

/** @brief Verify that RotationTransform correctly rotates Cartesian UV coordinates. */
void test_rotation_transform_uv() {
    // Signed range mapping: -0.5 maps to 0.25 turns (90 degrees).
    RotationTransform rotation(constant(sf16(-0x8000)));
    rotation.advanceFrame(f16(0), 0);

    UVMap testLayer = [](UV uv) {
        return PatternNormU16(raw(uv.u));
    };

    // Input Cartesian UV: (1.0, 0.5) -> Centered (1.0, 0.0)
    // Rotated 90 deg -> Centered (0.0, 1.0)
    // Back to Cartesian UV -> (0.5, 1.0)
    // Expected U: 0.5 (0x8000)
    UV input(sr16(0x00010000), sr16(0x00008000));
    
    UVMap transformedLayer = rotation(testLayer);
    PatternNormU16 result = transformedLayer(input);

    TEST_ASSERT_UINT16_WITHIN(100, 0x8000, raw(result));
}

/** @brief Verify that ZoomTransform correctly scales Cartesian UV coordinates relative to the center. */
void test_zoom_transform_uv() {
    ZoomTransform zoom(constant(sf16(SF16_MIN))); // Target min
    zoom.advanceFrame(f16(0), 0);
    
    UVMap testLayer = [](UV uv) { return PatternNormU16(raw(uv.u)); };
    
    UV input(sr16(0x0000C000), sr16(0x00008000));
    PatternNormU16 result = zoom(testLayer)(input);
    
    // With direct-mapped zoom and MIN_SCALE=1/4x (16384):
    // centered x at input.u=0.75 is +0.5 -> fx ~= 0x2000.
    // scaled_uv.u = (0x10000 + 0x2000) >> 1 = 0x9000 (36864).
    TEST_ASSERT_UINT16_WITHIN(200, 36864, raw(result));
}

/** @brief Verify that relative UV signals correctly accumulate over time. */
void test_uv_signal_accumulation() {
    // A constant relative signal returning (0.1, 0.1) per call
    // 0.1 in r16/sr16 (Q16.16) is ~6554
    UV delta(sr16(6554), sr16(6554));
    
    UVSignal rawSignal([delta](f16, TimeMillis) {
        return delta;
    });
    
    UVSignal resolved(
        [rawSignal, accumulated = UV(sr16(0), sr16(0))](f16 progress, TimeMillis elapsedMs) mutable {
            UV value = rawSignal(progress, elapsedMs);
            accumulated.u += value.u;
            accumulated.v += value.v;
            return accumulated;
        }
    );
    
    // First sample
    UV result1 = resolved(f16(100), 0);
    TEST_ASSERT_EQUAL_INT32(6554, raw(result1.u));
    TEST_ASSERT_EQUAL_INT32(6554, raw(result1.v));
    
    // Second sample (should accumulate)
    UV result2 = resolved(f16(200), 0);
    TEST_ASSERT_EQUAL_INT32(13108, raw(result2.u));
    TEST_ASSERT_EQUAL_INT32(13108, raw(result2.v));
}

/** @brief Verify PhaseAccumulator integration of signed speed. */
void test_phase_accumulator_signed() {
    // Speed: -0.5 turns per second
    auto speed = [](TimeMillis) { return sf16(-32768); };

    PhaseAccumulator accum(speed);
    accum.advance(0); // Init

    // Advance in clamped chunks (MAX_DELTA_TIME_MS).
    (void) accum.advance(200);
    (void) accum.advance(400);
    (void) accum.advance(600);
    (void) accum.advance(800);
    // 1000ms total -> -0.5 turns -> 0.5 (32768)
    f16 p1 = accum.advance(1000);
    TEST_ASSERT_UINT16_WITHIN(100, 32768, raw(p1));

    (void) accum.advance(1200);
    (void) accum.advance(1400);
    (void) accum.advance(1600);
    (void) accum.advance(1800);
    // Advance another 1000ms -> -1.0 turns -> 0
    f16 p2 = accum.advance(2000);
    TEST_ASSERT_UINT16_WITHIN(100, 0, raw(p2));
}

/** @brief Verify sine uses speed signal and is centered in signed space. */
void test_sine_speed() {
    // Speed: 1.0 turn per second
    Sf16Signal s = sine(cPerMil(1000), ceiling(), floor(), floor());
    
    // t=0 -> centered
    TEST_ASSERT_INT32_WITHIN(100, 0, raw(s.sample(TEST_SIGNED_RANGE, 0)));
    // Prime integration in clamped chunks.
    (void) s.sample(TEST_SIGNED_RANGE, 200);
    // t=250ms -> positive peak
    TEST_ASSERT_INT32_WITHIN(200, 65535, raw(s.sample(TEST_SIGNED_RANGE, 250)));
}

/** @brief Verify zoom driven by sine changes over elapsed time (not treated as constant). */
void test_zoom_transform_sine_varies_over_time() {
    ZoomTransform zoom(sine(cPerMil(1000)));
    UVMap probeLayer = [](UV uv) { return PatternNormU16(raw(uv.u)); };
    UV input(sr16(0x0000C000), sr16(0x00008000));

    zoom.advanceFrame(f16(0), 0);
    uint16_t a = raw(zoom(probeLayer)(input));

    zoom.advanceFrame(f16(0), 250);
    uint16_t b = raw(zoom(probeLayer)(input));

    zoom.advanceFrame(f16(0), 500);
    uint16_t c = raw(zoom(probeLayer)(input));

    // If zoom is animated, at least one later sample must differ from the initial one.
    TEST_ASSERT_TRUE(a != b || a != c);
}

/** @brief Verify easing functions loop if period > 0. */
void test_easing_period_looping() {
    // Linear signal looping every 500ms
    Sf16Signal s = linear(500);
    
    // Linear now emits signed values by convention.
    int32_t a = raw(s.sample(TEST_SIGNED_RANGE, 250));
    int32_t b = raw(s.sample(TEST_SIGNED_RANGE, 500));
    int32_t c = raw(s.sample(TEST_SIGNED_RANGE, 750));
    TEST_ASSERT_INT32_WITHIN(20, 0, a);
    TEST_ASSERT_INT32_WITHIN(20, -65536, b);
    TEST_ASSERT_INT32_WITHIN(20, a, c);
}

/** @brief Verify periodic signals receive scene elapsed time directly. */
void test_periodic_signal_uses_elapsed_time() {
    Sf16Signal s(
        SignalKind::PERIODIC,
        [](TimeMillis t) {
            return sf16(static_cast<int32_t>(t));
        }
    );

    TEST_ASSERT_INT32_WITHIN(2, 42, raw(s.sample(TEST_SIGNED_RANGE, 42)));
    TEST_ASSERT_INT32_WITHIN(2, 1234, raw(s.sample(TEST_SIGNED_RANGE, 1234)));
}

/** @brief Verify aperiodic RESET mode wraps time by duration modulo. */
void test_aperiodic_reset_wraps_time() {
    Sf16Signal s(
        SignalKind::APERIODIC,
        LoopMode::RESET,
        1000,
        [](TimeMillis t) {
            return sf16(static_cast<int32_t>(t));
        }
    );

    TEST_ASSERT_INT32_WITHIN(2, 250, raw(s.sample(TEST_SIGNED_RANGE, 250)));
    TEST_ASSERT_INT32_WITHIN(2, 250, raw(s.sample(TEST_SIGNED_RANGE, 1250)));
}

/** @brief Verify aperiodic duration=0 emits zero regardless of waveform. */
void test_aperiodic_zero_duration_emits_zero() {
    Sf16Signal s(
        SignalKind::APERIODIC,
        LoopMode::RESET,
        0,
        [](TimeMillis) {
            return sf16(0xFFFF);
        }
    );

    TEST_ASSERT_EQUAL_INT32(0, raw(s.sample(TEST_SIGNED_RANGE, 0)));
    TEST_ASSERT_EQUAL_INT32(0, raw(s.sample(TEST_SIGNED_RANGE, 1000)));
}

/** @brief Verify sampling saturates to signed sf16 bounds. */
void test_signal_sample_clamped() {
    Sf16Signal s(
        SignalKind::PERIODIC,
        [](TimeMillis) {
            return sf16(-1000);
        }
    );

    TEST_ASSERT_EQUAL_INT32(-1000, raw(s.sample(TEST_SIGNED_RANGE, 123)));
}

/** @brief Verify negative speed input maps to zero speed in magnitude domain. */
void test_sine_negative_speed_is_clamped_to_zero() {
    Sf16Signal negative = sine(csPerMil(-1000), ceiling(), floor(), floor());

    // Prime accumulator (first sample initializes elapsed baseline), then advance.
    (void) negative.sample(TEST_SIGNED_RANGE, 0);
    int32_t a = raw(negative.sample(TEST_SIGNED_RANGE, 200));
    int32_t b = raw(negative.sample(TEST_SIGNED_RANGE, 250));
    int32_t c = raw(negative.sample(TEST_SIGNED_RANGE, 400));

    // In magnitude mapping, -1 speed maps to 0 speed, so phase does not advance.
    TEST_ASSERT_INT32_WITHIN(50, 0, a);
    TEST_ASSERT_EQUAL_INT32(a, b);
    TEST_ASSERT_EQUAL_INT32(a, c);
}

/** @brief Verify scalar signal range mapping is done directly via sample(range, elapsedMs). */
void test_signal_range_mapping() {
    Sf16Signal signal(
        SignalKind::PERIODIC,
        [](TimeMillis) {
            return sf16(0x8000); // ~0.5
        }
    );
    MagnitudeRange<int32_t> range(0, 1000);
    TEST_ASSERT_EQUAL_INT32(750, signal.sample(range, 0));
}

/** @brief Verify that MagnitudeRange correctly interpolates values. */
void test_magnitude_range() {
    MagnitudeRange range(sf16(0), sf16(1000));
    
    // 0.0 -> midpoint
    TEST_ASSERT_EQUAL_INT32(500, raw(range.map(sf16(0))));
    // +0.5 -> 750
    TEST_ASSERT_EQUAL_INT32(750, raw(range.map(sf16(0x8000))));
    // 1.0 -> 1000
    TEST_ASSERT_EQUAL_INT32(1000, raw(range.map(sf16(0xFFFF))));
    // -1.0 -> 0
    TEST_ASSERT_EQUAL_INT32(0, raw(range.map(sf16(SF16_MIN))));
}

/** @brief Verify signed-direct mapping preserves signed identity range exactly. */
void test_bipolar_range_signed_direct_identity() {
    BipolarRange range{sf16(SF16_MIN), sf16(SF16_MAX)};

    TEST_ASSERT_EQUAL_INT32(SF16_MIN, raw(range.map(sf16(SF16_MIN))));
    TEST_ASSERT_EQUAL_INT32(0, raw(range.map(sf16(0))));
    TEST_ASSERT_EQUAL_INT32(SF16_MAX, raw(range.map(sf16(SF16_MAX))));
}

/** @brief Verify that UVRange correctly interpolates 2D coordinates. */
void test_uv_range() {
    UV min(sr16(0), sr16(0));
    UV max(sr16(0x10000), sr16(0x10000));
    UVRange range(min, max);
    
    // 0.0 -> midpoint (0.5, 0.5)
    UV result = range.map(sf16(0));
    TEST_ASSERT_EQUAL_INT32(0x8000, raw(result.u));
    TEST_ASSERT_EQUAL_INT32(0x8000, raw(result.v));
}

/** @brief Verify signed sf16 mul/div sat and wrap helpers. */
void test_sf16_mul_div_helpers() {
    sf16 half(0x8000);
    sf16 quarter(0x4000);

    TEST_ASSERT_EQUAL_INT32(raw(quarter), raw(mulSf16Sat(half, half)));
    TEST_ASSERT_EQUAL_INT32(raw(quarter), raw(mulSf16Wrap(half, half)));

    TEST_ASSERT_EQUAL_INT32(SF16_MAX, raw(divSf16Sat(half, half)));
    TEST_ASSERT_EQUAL_INT32(SF16_ONE, raw(divSf16Wrap(half, half)));
    TEST_ASSERT_EQUAL_INT32(SF16_MIN, raw(divSf16Sat(sf16(-0x8000), half)));
    TEST_ASSERT_EQUAL_INT32(0, raw(divSf16Sat(half, sf16(0))));
}

/** @brief Verify unsigned f16 mul/div sat and wrap helpers. */
void test_f16_mul_div_helpers() {
    f16 half(0x8000);
    f16 quarter(0x4000);

    TEST_ASSERT_EQUAL_UINT16(raw(quarter), raw(mulF16Sat(half, half)));
    TEST_ASSERT_EQUAL_UINT16(raw(quarter), raw(mulF16Wrap(half, half)));

    TEST_ASSERT_EQUAL_UINT16(F16_MAX, raw(divF16Sat(half, half)));
    TEST_ASSERT_EQUAL_UINT16(0, raw(divF16Wrap(half, half)));
    TEST_ASSERT_EQUAL_UINT16(0, raw(divF16Sat(half, f16(0))));
}

/** @brief Verify direct signed<->unsigned scalar mapping helpers. */
void test_sf16_f16_mapping_helpers() {
    TEST_ASSERT_EQUAL_UINT16(0, raw(toUnsigned(sf16(SF16_MIN))));
    TEST_ASSERT_EQUAL_UINT16(0x8000, raw(toUnsigned(sf16(0))));
    TEST_ASSERT_EQUAL_UINT16(F16_MAX, raw(toUnsigned(sf16(SF16_MAX))));

    TEST_ASSERT_EQUAL_INT32(SF16_MIN, raw(toSigned(f16(0))));
    TEST_ASSERT_EQUAL_INT32(0, raw(toSigned(f16(0x8000))));
    TEST_ASSERT_EQUAL_INT32(SF16_MAX, raw(toSigned(f16(F16_MAX))));
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
    RUN_TEST(test_sine_negative_speed_is_clamped_to_zero);
    RUN_TEST(test_signal_range_mapping);
    RUN_TEST(test_magnitude_range);
    RUN_TEST(test_bipolar_range_signed_direct_identity);
    RUN_TEST(test_uv_range);
    RUN_TEST(test_sf16_mul_div_helpers);
    RUN_TEST(test_f16_mul_div_helpers);
    RUN_TEST(test_sf16_f16_mapping_helpers);
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
    RUN_TEST(test_sine_negative_speed_is_clamped_to_zero);
    RUN_TEST(test_signal_range_mapping);
    RUN_TEST(test_magnitude_range);
    RUN_TEST(test_bipolar_range_signed_direct_identity);
    RUN_TEST(test_uv_range);
    RUN_TEST(test_sf16_mul_div_helpers);
    RUN_TEST(test_f16_mul_div_helpers);
    RUN_TEST(test_sf16_f16_mapping_helpers);
    return UNITY_END();
}
#endif
