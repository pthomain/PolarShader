//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#include <unity.h>
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/signals/SignalSamplers.h"
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
#endif

using namespace PolarShader;

namespace {
    const BipolarRange<sf16> &SIGNED_RANGE = bipolarRange();
}

/** @brief Verify that sine wave oscillates between -1 and 1. */
void test_sine_waveform() {
    Sf16Signal s = sine(constant(1000), constant(1000), constant(500), constant(0));
    
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
    Sf16Signal s = triangle(constant(1000), constant(1000), constant(500), constant(0));
    
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
    Sf16Signal s = square(constant(1000), constant(1000), constant(500), constant(0));
    
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
    Sf16Signal s = sawtooth(constant(1000), constant(1000), constant(500), constant(0));
    
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

/** @brief Verify amplitude modulation. */
void test_amplitude_modulation() {
    // Constant signal with 0.5 amplitude
    // Using constant(500) which maps to 0.5 in MagnitudeRange
    Sf16Signal s = sine(constant(1000), constant(500), constant(500), constant(0));
    
    // t=0 -> Initialize
    (void) s.sample(SIGNED_RANGE, 0);

    // At peak (250ms), result should be 0.5 (32768)
    (void) s.sample(SIGNED_RANGE, 150);
    TEST_ASSERT_INT32_WITHIN(1000, 32768, raw(s.sample(SIGNED_RANGE, 250)));
}

/** @brief Verify threshold modulation. */
void test_threshold_modulation() {
    // Sine with 0.5 amplitude and 0.5 threshold -> oscillating between 0 and 1
    // Threshold is signed, so use constant(750) for 0.5?
    // Wait. constant(p) = (p/1000) * 2 - 1.
    // We want 0.5. (p/1000) * 2 - 1 = 0.5.
    // (p/1000) * 2 = 1.5.
    // p/1000 = 0.75.
    // p = 750.
    Sf16Signal s = sine(constant(1000), constant(500), constant(750), constant(0));
    
    // t=0 -> Initialize, result is threshold = 0.5 (32768)
    TEST_ASSERT_INT32_WITHIN(500, 32768, raw(s.sample(SIGNED_RANGE, 0)));
    
    (void) s.sample(SIGNED_RANGE, 150);
    // At peak (250ms), result is 1.0 (SF16_MAX)
    TEST_ASSERT_INT32_WITHIN(1000, SF16_MAX, raw(s.sample(SIGNED_RANGE, 250)));
    
    (void) s.sample(SIGNED_RANGE, 400);
    (void) s.sample(SIGNED_RANGE, 600);
    // At trough (750ms), result is 0.0 (0)
    TEST_ASSERT_INT32_WITHIN(1000, 0, raw(s.sample(SIGNED_RANGE, 750)));
}

/** @brief Verify that zero speed signal results in no phase advance. */
void test_zero_speed_signal() {
    Sf16Signal s = sine(constant(500), constant(1000), constant(500), constant(0));
    
    (void) s.sample(SIGNED_RANGE, 0);
    int32_t a = raw(s.sample(SIGNED_RANGE, 500));
    int32_t b = raw(s.sample(SIGNED_RANGE, 1000));
    
    TEST_ASSERT_EQUAL_INT32(a, b);
}

/** @brief Verify handling of negative time (e.g. scene loop/reset). */
void test_negative_time_reset() {
    Sf16Signal s = sine(constant(1000), constant(1000), constant(500), constant(0));
    
    (void) s.sample(SIGNED_RANGE, 500);
    // t=200 is "negative" relative to last sample.
    // PhaseAccumulator should handle it gracefully (clamping delta or resetting).
    (void) s.sample(SIGNED_RANGE, 200);
    
    // It shouldn't crash.
    TEST_ASSERT_TRUE(true);
}

/** @brief Verify extreme amplitude and threshold. */
void test_extreme_modulation_clamping() {
    // Amplitude = 2.0 (out of bounds), Threshold = 1.0
    // Result should be clamped to SF16_MAX.
    // constant(1000) is 1.0. 
    Sf16Signal s = sine(constant(1000), constant(1000), constant(1000), constant(0));
    
    (void) s.sample(SIGNED_RANGE, 0);
    (void) s.sample(SIGNED_RANGE, 250);
    int32_t a = raw(s.sample(SIGNED_RANGE, 250));
    
    TEST_ASSERT_EQUAL_INT32(SF16_MAX, a);
}

#ifdef ARDUINO
void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_sine_waveform);
    RUN_TEST(test_triangle_waveform);
    RUN_TEST(test_square_waveform);
    RUN_TEST(test_sawtooth_waveform);
    RUN_TEST(test_amplitude_modulation);
    RUN_TEST(test_threshold_modulation);
    RUN_TEST(test_zero_speed_signal);
    RUN_TEST(test_negative_time_reset);
    RUN_TEST(test_extreme_modulation_clamping);
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
    RUN_TEST(test_amplitude_modulation);
    RUN_TEST(test_threshold_modulation);
    RUN_TEST(test_zero_speed_signal);
    RUN_TEST(test_negative_time_reset);
    RUN_TEST(test_extreme_modulation_clamping);
    return UNITY_END();
}
#endif
