//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2023 Pierre Thomain

/*
 * This file is part of LED Segments.
 *
 * LED Segments is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LED Segments is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LED Segments. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "MathUtils.h"
#include "Units.h"
#include "NoiseUtils.h"
#include "polar/pipeline/signals/modulators/AngularModulators.h"
#include "polar/pipeline/signals/modulators/ScalarModulators.h"
#include "polar/pipeline/signals/motion/linear/LinearMotion.h"

namespace LEDSegments::UnitsTest {
    inline bool anglePromotionTest() {
        const AngleUnitsQ0_16 angles[] = {
            AngleUnitsQ0_16(0),
            AngleUnitsQ0_16(QUARTER_TURN_U16),
            AngleUnitsQ0_16(HALF_TURN_U16),
            AngleUnitsQ0_16(static_cast<uint16_t>(HALF_TURN_U16 + QUARTER_TURN_U16))
        };
        for (const auto &a: angles) {
            AngleTurnsUQ16_16 p = angleUnitsToAngleTurns(a);
            AngleUnitsQ0_16 roundtrip = angleTurnsToAngleUnits(p);
            if (raw(roundtrip) != raw(a)) return false;
        }
        return true;
    }

    inline bool trigSamplingTest() {
        const uint16_t samples[] = {0, QUARTER_TURN_U16, HALF_TURN_U16, 49152u};
        for (uint16_t sample_raw: samples) {
            AngleUnitsQ0_16 a(sample_raw);
            int16_t direct = sin16(sample_raw);
            int16_t wrapped = raw(sinQ1_15(a));
            if (direct != wrapped) return false;
        }
        return true;
    }

    inline bool phaseTrigSamplingTest() {
        const uint16_t samples[] = {0, QUARTER_TURN_U16, HALF_TURN_U16, 49152u};
        for (uint16_t raw_angle: samples) {
            AngleUnitsQ0_16 a(raw_angle);
            AngleTurnsUQ16_16 p = angleUnitsToAngleTurns(a);
            int16_t direct = sin16(toFastLedPhase(p));
            int16_t wrapped = raw(sinQ1_15(p));
            if (direct != wrapped) return false;
        }
        return true;
    }

    inline bool phaseAccumulatorSmoothnessTest() {
        auto phase = IntegrateAngle(Constant(FracQ16_16::fromRaw(Q16_16_ONE / 4)));
        phase(0);
        AngleTurnsUQ16_16 p1 = phase(16);
        AngleTurnsUQ16_16 p2 = phase(32);
        return ((raw(p1) & 0xFFFFu) != 0u) || ((raw(p2) & 0xFFFFu) != 0u);
    }

    inline bool phaseVelocityUnitTest() {
        auto phase = IntegrateAngle(ConstantPhaseVelocity(FracQ16_16::fromRaw(Q16_16_ONE))); // 1 turn/sec
        const TimeMillis samples[] = {0, 200, 400, 600, 800, 1000};
        AngleTurnsUQ16_16 current = AngleTurnsUQ16_16(0);
        for (TimeMillis t: samples) {
            current = phase(t);
        }
        return raw(current) == raw(ANGLE_TURNS_ONE_TURN);
    }

    inline bool linearMotionAxisTest() {
        FracQ16_16 speed_value = FracQ16_16::fromRaw(Q16_16_ONE); // 1 unit/sec
        auto speed = Constant(speed_value);
        auto direction = ConstantAngleUnits(AngleUnitsQ0_16(0));
        UnboundedLinearMotion motion(FracQ16_16(0), FracQ16_16(0), speed, direction);
        const TimeMillis samples[] = {0, 200, 400, 600, 800, 1000};
        for (TimeMillis t: samples) {
            motion.advanceFrame(t);
        }
        FracQ16_16 dt = millisToQ16_16(200);
        FracQ16_16 step_distance = mul_q16_16_sat(speed_value, dt);
        int64_t step_dx = scale_q16_16_by_trig(RawQ16_16(step_distance.asRaw()),
                                               cosQ1_15(AngleTurnsUQ16_16(0)));
        int32_t expected = static_cast<int32_t>(step_dx * 5);
        return raw(motion.getRawX()) == expected && raw(motion.getRawY()) == 0;
    }

    inline bool linearMotionQuarterTurnTest() {
        FracQ16_16 speed_value = FracQ16_16::fromRaw(Q16_16_ONE); // 1 unit/sec
        auto speed = Constant(speed_value);
        AngleUnitsQ0_16 quarter(QUARTER_TURN_U16);
        auto direction = ConstantAngleUnits(quarter);
        UnboundedLinearMotion motion(FracQ16_16(0), FracQ16_16(0), speed, direction);
        const TimeMillis samples[] = {0, 200, 400, 600, 800, 1000};
        for (TimeMillis t: samples) {
            motion.advanceFrame(t);
        }
        AngleTurnsUQ16_16 phase = angleUnitsToAngleTurns(quarter);
        FracQ16_16 dt = millisToQ16_16(200);
        FracQ16_16 step_distance = mul_q16_16_sat(speed_value, dt);
        int64_t step_dy = scale_q16_16_by_trig(RawQ16_16(step_distance.asRaw()), sinQ1_15(phase));
        int32_t expected_y = static_cast<int32_t>(step_dy * 5);
        int32_t x_raw = raw(motion.getRawX());
        int32_t x_abs = x_raw < 0 ? -x_raw : x_raw;
        return x_abs <= 4 && raw(motion.getRawY()) == expected_y;
    }

    inline bool linearMotionQuantizationTest() {
        auto speed = Constant(FracQ16_16::fromRaw(256)); // small but non-zero speed
        auto direction = ConstantAngleUnits(AngleUnitsQ0_16(0));
        UnboundedLinearMotion motion(FracQ16_16(0), FracQ16_16(0), speed, direction);
        TimeMillis t = 0;
        for (int i = 0; i < 100; ++i) {
            motion.advanceFrame(t);
            t += 16;
        }
        return raw(motion.getRawX()) != 0;
    }

    inline bool clampQ16RangeTest() {
        int64_t raw_value = static_cast<int64_t>(INT32_MAX) + 10;
        FracQ16_16 result = clamp_q16_16_raw(raw_value);
        return result.asRaw() == INT32_MAX;
    }

    inline bool wrapAddSignedTest() {
        AngleTurnsUQ16_16 p = AngleTurnsUQ16_16(0);
        AngleTurnsUQ16_16 r = wrapAddSigned(p, -static_cast<int32_t>(1 << 16));
        return raw(r) > raw(p);
    }

    inline bool noiseNormalizationTest() {
        NoiseRawU16 noise_raw(U16_HALF);
        NoiseNormU16 norm = normaliseNoise16(noise_raw);
        return raw(norm) <= FRACT_Q0_16_MAX;
    }

    inline bool runStrongTypeTests() {
        return anglePromotionTest()
               && trigSamplingTest()
               && phaseTrigSamplingTest()
               && phaseAccumulatorSmoothnessTest()
               && phaseVelocityUnitTest()
               && linearMotionAxisTest()
               && linearMotionQuarterTurnTest()
               && linearMotionQuantizationTest()
               && clampQ16RangeTest()
               && wrapAddSignedTest()
               && noiseNormalizationTest();
    }
} // namespace LEDSegments::UnitsTest
