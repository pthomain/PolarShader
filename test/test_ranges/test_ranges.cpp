//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#include <unity.h>
#include "renderer/pipeline/signals/ranges/AngleRange.h"
#include "renderer/pipeline/signals/ranges/MagnitudeRange.h"
#include "renderer/pipeline/signals/ranges/BipolarRange.h"
#include "renderer/pipeline/signals/ranges/UVRange.h"

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif

using namespace PolarShader;

/** @brief Verify AngleRange wrapping for values > 1.0 (SF16_MAX). */
void test_angle_range_wrap_positive() {
    AngleRange range(f16(0), f16(F16_MAX));
    
    // SF16_MAX is ~1.0, should map to ~F16_MAX
    // Note: Due to fixed-point scaling (65535 * 65535 >> 16), result is 65534.
    TEST_ASSERT_UINT16_WITHIN(1, F16_MAX, raw(range.map(sf16(SF16_MAX))));
    
    // Test with raw value > SF16_MAX (out of bounds)
    // toUnsignedWrapped should wrap it.
    // SF16_MAX + 1 is 65536. (65536 + 65536) / 2 = 65536. uint16_t(65536) = 0.
    TEST_ASSERT_EQUAL_UINT16(0, raw(range.map(sf16(65536))));
}

/** @brief Verify AngleRange wrapping behavior when min > max (inverted span). */
void test_angle_range_inverted_span() {
    // Range from 270 (0xC000) to 90 (0x4000) degrees. Span = 180 deg (0x8000).
    AngleRange range(f16(0xC000), f16(0x4000));
    
    // t=0 (midpoint of sf16 -> 0x8000 unsigned) -> 0xC000 + 0x4000 = 0x10000 -> 0x0000
    TEST_ASSERT_EQUAL_UINT16(0x0000, raw(range.map(sf16(0))));
    
    // t=1.0 (SF16_MAX -> 0xFFFF unsigned) -> 0xC000 + ~0x8000 = ~0x14000 -> ~0x4000
    TEST_ASSERT_UINT16_WITHIN(10, 0x4000, raw(range.map(sf16(SF16_MAX))));
    
    // t=-1.0 (SF16_MIN -> 0x0000 unsigned) -> 0xC000
    TEST_ASSERT_EQUAL_UINT16(0xC000, raw(range.map(sf16(SF16_MIN))));
}

/** @brief Verify MagnitudeRange clamping for out-of-bounds inputs. */
void test_magnitude_range_clamping() {
    MagnitudeRange<sf16> range(sf16(0), sf16(1000));
    
    // Lower bound
    TEST_ASSERT_EQUAL_INT32(0, raw(range.map(sf16(SF16_MIN))));
    TEST_ASSERT_EQUAL_INT32(0, raw(range.map(sf16(SF16_MIN - 1))));
    
    // Upper bound
    TEST_ASSERT_EQUAL_INT32(1000, raw(range.map(sf16(SF16_MAX))));
    TEST_ASSERT_EQUAL_INT32(1000, raw(range.map(sf16(SF16_MAX + 1))));
}

/** @brief Verify BipolarRange clamping for out-of-bounds inputs. */
void test_bipolar_range_clamping() {
    BipolarRange<sf16> range(sf16(-500), sf16(500));
    
    // Lower bound
    TEST_ASSERT_EQUAL_INT32(-500, raw(range.map(sf16(SF16_MIN))));
    TEST_ASSERT_EQUAL_INT32(-500, raw(range.map(sf16(SF16_MIN - 1))));
    
    // Upper bound
    TEST_ASSERT_EQUAL_INT32(500, raw(range.map(sf16(SF16_MAX))));
    TEST_ASSERT_EQUAL_INT32(500, raw(range.map(sf16(SF16_MAX + 1))));
}

/** @brief Verify UVRange clamping for out-of-bounds inputs. */
void test_uv_range_clamping() {
    UV min(sr16(0), sr16(0));
    UV max(sr16(1000), sr16(2000));
    UVRange range(min, max);
    
    // Lower bound (SF16_MIN -> 0.0)
    UV res_min = range.map(sf16(SF16_MIN - 100));
    TEST_ASSERT_EQUAL_INT32(0, raw(res_min.u));
    TEST_ASSERT_EQUAL_INT32(0, raw(res_min.v));
    
    // Upper bound (SF16_MAX -> 1.0)
    UV res_max = range.map(sf16(SF16_MAX + 100));
    TEST_ASSERT_INT32_WITHIN(1, 1000, raw(res_max.u));
    TEST_ASSERT_INT32_WITHIN(1, 2000, raw(res_max.v));
}

#ifdef ARDUINO
void setup() {
    delay(2000);
    UNITY_BEGIN();
    RUN_TEST(test_angle_range_wrap_positive);
    RUN_TEST(test_angle_range_inverted_span);
    RUN_TEST(test_magnitude_range_clamping);
    RUN_TEST(test_bipolar_range_clamping);
    RUN_TEST(test_uv_range_clamping);
    UNITY_END();
}
void loop() {}
#else
int main() {
    UNITY_BEGIN();
    RUN_TEST(test_angle_range_wrap_positive);
    RUN_TEST(test_angle_range_inverted_span);
    RUN_TEST(test_magnitude_range_clamping);
    RUN_TEST(test_bipolar_range_clamping);
    RUN_TEST(test_uv_range_clamping);
    return UNITY_END();
}
#endif
