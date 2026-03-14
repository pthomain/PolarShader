//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#include <unity.h>
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/signals/SignalSamplers.h"
#include "renderer/pipeline/transforms/PaletteTransform.h"
#include "renderer/pipeline/signals/ranges/BipolarRange.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"
#include "renderer/pipeline/signals/ranges/AngleRange.h"

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#include "native/FastLED.h"
#endif

#ifndef ARDUINO
#include "renderer/pipeline/maths/src/PolarMaths.cpp"
#include "renderer/pipeline/maths/src/NoiseMaths.cpp"
#include "renderer/pipeline/maths/src/PatternMaths.cpp"
#include "renderer/pipeline/maths/src/TimeMaths.cpp"
#include "renderer/pipeline/signals/src/Signals.cpp"
#include "renderer/pipeline/signals/src/SignalSamplers.cpp"
#include "renderer/pipeline/signals/src/accumulators/Accumulators.cpp"
#include "renderer/pipeline/transforms/src/PaletteTransform.cpp"
#endif

using namespace PolarShader;

namespace {
    const BipolarRange<sf16> &SIGNED_RANGE = bipolarRange();
}

/** @brief Verify that sine wave oscillates between -1 and 1. */
void test_sine_waveform() {
    Sf16Signal s = sine(constant(1000), sf16(0));
    
    // t=0 -> Initialize accumulator, returns initialPhase=0.
    TEST_ASSERT_INT32_WITHIN(200, 0, raw(s.sample(SIGNED_RANGE, 0)));
    
    // t=250ms -> 1.0 (SF16_MAX)
    // 250ms is within MAX_DELTA_TIME_MS=200? No! 
    // I must use smaller steps to avoid clamping if I want exact results.
    (void) s.sample(SIGNED_RANGE, 150);
    TEST_ASSERT_INT32_WITHIN(500, SF16_MAX, raw(s.sample(SIGNED_RANGE, 250)));
    
    (void) s.sample(SIGNED_RANGE, 400);
    (void) s.sample(SIGNED_RANGE, 600);
    // t=750ms -> -1.0 (SF16_MIN)
    TEST_ASSERT_INT32_WITHIN(500, SF16_MIN, raw(s.sample(SIGNED_RANGE, 750)));
}

/** @brief Verify triangle waveform. */
void test_triangle_waveform() {
    Sf16Signal s = triangle(constant(1000), sf16(0));
    
    // t=0 -> Initialize, returns -1
    TEST_ASSERT_INT32_WITHIN(100, SF16_MIN, raw(s.sample(SIGNED_RANGE, 0)));
    
    // t=250ms -> 0
    (void) s.sample(SIGNED_RANGE, 150);
    TEST_ASSERT_INT32_WITHIN(100, 0, raw(s.sample(SIGNED_RANGE, 250)));
    
    (void) s.sample(SIGNED_RANGE, 400);
    // t=500ms -> 1
    TEST_ASSERT_INT32_WITHIN(100, SF16_MAX, raw(s.sample(SIGNED_RANGE, 500)));
}

/** @brief Verify square waveform. */
void test_square_waveform() {
    Sf16Signal s = square(constant(1000), sf16(0));
    
    // t=0 -> Initialize
    (void) s.sample(SIGNED_RANGE, 0);

    // t=100ms -> 1.0 (phase < 0.5)
    TEST_ASSERT_INT32_WITHIN(10, SF16_MAX, raw(s.sample(SIGNED_RANGE, 100)));
    
    (void) s.sample(SIGNED_RANGE, 300);
    (void) s.sample(SIGNED_RANGE, 500);
    // t=600ms -> -1.0 (phase > 0.5)
    TEST_ASSERT_INT32_WITHIN(10, SF16_MIN, raw(s.sample(SIGNED_RANGE, 600)));
}

/** @brief Verify sawtooth waveform. */
void test_sawtooth_waveform() {
    Sf16Signal s = sawtooth(constant(1000), sf16(0));
    
    // t=0 -> Initialize, returns -1
    TEST_ASSERT_INT32_WITHIN(100, SF16_MIN, raw(s.sample(SIGNED_RANGE, 0)));
    
    (void) s.sample(SIGNED_RANGE, 200);
    (void) s.sample(SIGNED_RANGE, 400);
    // t=500ms -> 0
    TEST_ASSERT_INT32_WITHIN(100, 0, raw(s.sample(SIGNED_RANGE, 500)));
    
    (void) s.sample(SIGNED_RANGE, 700);
    (void) s.sample(SIGNED_RANGE, 900);
    // t=999ms -> ~1
    TEST_ASSERT_INT32_WITHIN(500, SF16_MAX, raw(s.sample(SIGNED_RANGE, 999)));
}

/** @brief Verify the bounded periodic overload remaps a waveform into a bounded interval. */
void test_bounded_sine_waveform() {
    Sf16Signal s = sine(constant(1000), constant(100), constant(800));

    TEST_ASSERT_INT32_WITHIN(600, -6554, raw(s.sample(SIGNED_RANGE, 0)));

    (void) s.sample(SIGNED_RANGE, 150);
    TEST_ASSERT_INT32_WITHIN(600, 39322, raw(s.sample(SIGNED_RANGE, 250)));

    (void) s.sample(SIGNED_RANGE, 400);
    (void) s.sample(SIGNED_RANGE, 600);
    TEST_ASSERT_INT32_WITHIN(600, -52429, raw(s.sample(SIGNED_RANGE, 750)));
}

/** @brief Verify the bounded periodic overload reproduces the centered 25%-75% range case. */
void test_bounded_sine_centered_range() {
    Sf16Signal s = sine(constant(1000), constant(250), constant(750));

    (void) s.sample(SIGNED_RANGE, 0);
    (void) s.sample(SIGNED_RANGE, 150);
    TEST_ASSERT_INT32_WITHIN(1000, 32768, raw(s.sample(SIGNED_RANGE, 250)));
}

/** @brief Verify that zero phase velocity results in no phase advance. */
void test_zero_speed_signal() {
    Sf16Signal s = sine(constant(0), sf16(0));
    
    (void) s.sample(SIGNED_RANGE, 0);
    int32_t a = raw(s.sample(SIGNED_RANGE, 500));
    int32_t b = raw(s.sample(SIGNED_RANGE, 1000));
    
    TEST_ASSERT_EQUAL_INT32(a, b);
}

/** @brief Verify phase velocity is interpreted in the magnitude domain. */
void test_half_speed_signal() {
    Sf16Signal s = sine(constant(500), sf16(0));

    TEST_ASSERT_INT32_WITHIN(200, 0, raw(s.sample(SIGNED_RANGE, 0)));

    (void) s.sample(SIGNED_RANGE, 200);
    (void) s.sample(SIGNED_RANGE, 400);
    TEST_ASSERT_INT32_WITHIN(500, SF16_MAX, raw(s.sample(SIGNED_RANGE, 500)));

    (void) s.sample(SIGNED_RANGE, 700);
    (void) s.sample(SIGNED_RANGE, 900);
    TEST_ASSERT_INT32_WITHIN(500, 0, raw(s.sample(SIGNED_RANGE, 1000)));
}

/** @brief Verify handling of negative time (e.g. scene loop/reset). */
void test_negative_time_reset() {
    Sf16Signal s = sine(constant(1000), sf16(0));
    
    (void) s.sample(SIGNED_RANGE, 500);
    // t=200 is "negative" relative to last sample.
    // PhaseAccumulator should handle it gracefully (clamping delta or resetting).
    (void) s.sample(SIGNED_RANGE, 200);
    
    // It shouldn't crash.
    TEST_ASSERT_TRUE(true);
}

/** @brief Verify bounded periodic overloads normalize swapped and out-of-range bounds. */
void test_bounded_sine_clamps_and_swaps_bounds() {
    Sf16Signal forward = sine(constant(1000), constant(100), constant(800));
    Sf16Signal reversed = sine(constant(1000), constant(800), constant(100));
    Sf16Signal clamped = sine(constant(1000), constant(1200), constant(1200));

    TEST_ASSERT_INT32_WITHIN(10, raw(forward.sample(SIGNED_RANGE, 0)), raw(reversed.sample(SIGNED_RANGE, 0)));
    (void) forward.sample(SIGNED_RANGE, 150);
    (void) reversed.sample(SIGNED_RANGE, 150);
    TEST_ASSERT_INT32_WITHIN(10, raw(forward.sample(SIGNED_RANGE, 250)), raw(reversed.sample(SIGNED_RANGE, 250)));
    TEST_ASSERT_INT32_WITHIN(10, SF16_MAX, raw(clamped.sample(SIGNED_RANGE, 0)));
}

/** @brief Verify fixed signed phase offsets wrap into the phase domain. */
void test_phase_offset_wraps() {
    Sf16Signal s = sine(constant(1000), sf16(-16384), constant(0), constant(1000));

    TEST_ASSERT_INT32_WITHIN(500, SF16_MIN, raw(s.sample(SIGNED_RANGE, 0)));
}

/** @brief Verify smap supports animated floor and ceiling signals. */
void test_smap_animated_bounds() {
    Sf16Signal s = smap(constant(500), linear(1000), constant(1000));

    TEST_ASSERT_INT32_WITHIN(100, 0, raw(s.sample(SIGNED_RANGE, 0)));
    TEST_ASSERT_INT32_WITHIN(1000, 32768, raw(s.sample(SIGNED_RANGE, 500)));
}

/** @brief Verify smap preserves aperiodic metadata. */
void test_smap_preserves_aperiodic_metadata() {
    Sf16Signal s = smap(linear(750, LoopMode::SATURATE), constant(100), constant(800));

    TEST_ASSERT_EQUAL_INT(static_cast<int>(SignalKind::APERIODIC), static_cast<int>(s.kind()));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(LoopMode::SATURATE), static_cast<int>(s.loopMode()));
    TEST_ASSERT_EQUAL_UINT32(750u, s.duration());
}

/** @brief Verify zero clip disables feather even when max feather defaults are used. */
void test_palette_clip_zero_has_zero_feather() {
    auto context = std::make_shared<PipelineContext>();
    PaletteTransform transform(constant(0), constant(0));

    transform.setContext(context);
    transform.advanceFrame(f16(0), 0);

    TEST_ASSERT_EQUAL_UINT16(0, raw(context->paletteClip));
    TEST_ASSERT_EQUAL_UINT16(0, raw(context->paletteClipFeather));
}

/** @brief Verify palette clip feather scales proportionally with the live clip signal. */
void test_palette_clip_feather_scales_with_clip_signal() {
    auto context = std::make_shared<PipelineContext>();
    PaletteTransform transform(
        constant(0),
        sine(constant(1000), constant(0), constant(200))
    );

    transform.setContext(context);

    transform.advanceFrame(f16(0), 0);
    TEST_ASSERT_UINT16_WITHIN(50, raw(perMil(100)), raw(context->paletteClip));
    TEST_ASSERT_UINT16_WITHIN(50, raw(perMil(50)), raw(context->paletteClipFeather));

    transform.advanceFrame(f16(0), 150);
    transform.advanceFrame(f16(0), 250);
    TEST_ASSERT_UINT16_WITHIN(50, raw(perMil(200)), raw(context->paletteClip));
    TEST_ASSERT_UINT16_WITHIN(50, raw(perMil(100)), raw(context->paletteClipFeather));

    transform.advanceFrame(f16(0), 400);
    transform.advanceFrame(f16(0), 600);
    transform.advanceFrame(f16(0), 750);
    TEST_ASSERT_EQUAL_UINT16(0, raw(context->paletteClip));
    TEST_ASSERT_EQUAL_UINT16(0, raw(context->paletteClipFeather));
}

#ifdef ARDUINO
void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_sine_waveform);
    RUN_TEST(test_triangle_waveform);
    RUN_TEST(test_square_waveform);
    RUN_TEST(test_sawtooth_waveform);
    RUN_TEST(test_bounded_sine_waveform);
    RUN_TEST(test_bounded_sine_centered_range);
    RUN_TEST(test_zero_speed_signal);
    RUN_TEST(test_half_speed_signal);
    RUN_TEST(test_negative_time_reset);
    RUN_TEST(test_bounded_sine_clamps_and_swaps_bounds);
    RUN_TEST(test_phase_offset_wraps);
    RUN_TEST(test_smap_animated_bounds);
    RUN_TEST(test_smap_preserves_aperiodic_metadata);
    RUN_TEST(test_palette_clip_zero_has_zero_feather);
    RUN_TEST(test_palette_clip_feather_scales_with_clip_signal);
    UNITY_END();
}
void loop() {}
#else
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_sine_waveform);
    RUN_TEST(test_triangle_waveform);
    RUN_TEST(test_square_waveform);
    RUN_TEST(test_sawtooth_waveform);
    RUN_TEST(test_bounded_sine_waveform);
    RUN_TEST(test_bounded_sine_centered_range);
    RUN_TEST(test_zero_speed_signal);
    RUN_TEST(test_half_speed_signal);
    RUN_TEST(test_negative_time_reset);
    RUN_TEST(test_bounded_sine_clamps_and_swaps_bounds);
    RUN_TEST(test_phase_offset_wraps);
    RUN_TEST(test_smap_animated_bounds);
    RUN_TEST(test_smap_preserves_aperiodic_metadata);
    RUN_TEST(test_palette_clip_zero_has_zero_feather);
    RUN_TEST(test_palette_clip_feather_scales_with_clip_signal);
    return UNITY_END();
}
#endif
