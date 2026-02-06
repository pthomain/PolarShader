//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#ifdef ARDUINO
#include <Arduino.h>
#endif
#include <unity.h>
#include "renderer/pipeline/units/ScalarUnits.h"
#include "renderer/pipeline/units/UVUnits.h"

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

#ifdef ARDUINO
void setup() {
    delay(2000); 
    UNITY_BEGIN();
    RUN_TEST(test_frac_q16_16_raw_values);
    RUN_TEST(test_uv_coordinate_structure);
    UNITY_END();
}

void loop() {}
#else
int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_frac_q16_16_raw_values);
    RUN_TEST(test_uv_coordinate_structure);
    return UNITY_END();
}
#endif
