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
    Sf16Signal s = sine(csPerMil(1000), ceiling(), midPoint(), floor());
    
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
    Sf16Signal s = triangle(csPerMil(1000), ceiling(), midPoint(), floor());
    
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
    Sf16Signal s = square(csPerMil(1000), ceiling(), midPoint(), floor());
    
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
    Sf16Signal s = sawtooth(csPerMil(1000), ceiling(), midPoint(), floor());
    
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
    Sf16Signal s = sine(csPerMil(1000), cPerMil(500), midPoint(), floor());
    
    // t=0 -> Initialize
    (void) s.sample(SIGNED_RANGE, 0);

    // At peak (250ms), result should be 0.5 (32768)
    (void) s.sample(SIGNED_RANGE, 150);
    TEST_ASSERT_INT32_WITHIN(1000, 32768, raw(s.sample(SIGNED_RANGE, 250)));
}

/** @brief Verify offset modulation. */
void test_offset_modulation() {
    // Sine with 0.5 amplitude and 0.5 offset -> oscillating between 0 and 1
    Sf16Signal s = sine(csPerMil(1000), cPerMil(500), csPerMil(500), floor());
    
    // t=0 -> Initialize, result is offset = 0.5 (32768)
    TEST_ASSERT_INT32_WITHIN(500, 32768, raw(s.sample(SIGNED_RANGE, 0)));
    
    (void) s.sample(SIGNED_RANGE, 150);
    // At peak (250ms), result is 1.0 (SF16_MAX)
    TEST_ASSERT_INT32_WITHIN(1000, SF16_MAX, raw(s.sample(SIGNED_RANGE, 250)));
    
    (void) s.sample(SIGNED_RANGE, 400);
    (void) s.sample(SIGNED_RANGE, 600);
    // At trough (750ms), result is 0.0 (0)
    TEST_ASSERT_INT32_WITHIN(1000, 0, raw(s.sample(SIGNED_RANGE, 750)));
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
    RUN_TEST(test_offset_modulation);
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
    RUN_TEST(test_offset_modulation);
    return UNITY_END();
}
#endif
