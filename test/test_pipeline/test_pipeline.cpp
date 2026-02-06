//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

/*
 * This file is part of PolarShader.
 *
 * PolarShader is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PolarShader is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PolarShader. If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "native/Arduino.h"
#endif
#include <unity.h>
#include "renderer/pipeline/ranges/PolarRange.h"
#include "renderer/pipeline/ranges/CartRange.h"
#include "renderer/pipeline/signals/Signals.h"

using namespace PolarShader;

void test_range_wraps_across_zero() {
    PolarRange range(FracQ0_16(0xC000u), FracQ0_16(0x4000u));
    TEST_ASSERT_EQUAL_UINT16(0xC000u, raw(range.map(SFracQ0_16(0)).get()));
    TEST_ASSERT_EQUAL_UINT16(0x0000u, raw(range.map(SFracQ0_16(0x8000)).get()));
}

void test_cartesian_motion_uses_direction_and_velocity() {
    SFracQ0_16Signal direction = constant(SFracQ0_16(0));
    SFracQ0_16Signal velocity = constant(SFracQ0_16(0x8000));
    
    // Direction is mapped to Angle (turns)
    auto mappedDirection = PolarRange().mapSignal(direction);
    // Speed is mapped to units/sec
    auto mappedSpeed = ScalarRange(0, 1000).mapSignal(velocity);
    
    CartesianMotionAccumulator acc(SPoint32{0, 0}, mappedDirection, mappedSpeed);

    acc.advance(0);
    SPoint32 pos = acc.advance(100);

    TEST_ASSERT_TRUE(pos.x > 0);
    TEST_ASSERT_INT_WITHIN(2, 0, pos.y);
}

#ifdef ARDUINO
void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_range_wraps_across_zero);
    RUN_TEST(test_cartesian_motion_uses_direction_and_velocity);
    UNITY_END();
}

void loop() {
}
#else
int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_range_wraps_across_zero);
    RUN_TEST(test_cartesian_motion_uses_direction_and_velocity);
    return UNITY_END();
}
#endif
