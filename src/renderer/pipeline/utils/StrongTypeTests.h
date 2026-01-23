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

#pragma once

#include "MathUtils.h"
#include "renderer/pipeline/modulators/BoundUtils.h"
#include "Units.h"
#include "NoiseUtils.h"
#include "renderer/pipeline/modulators/signals/AngularSignals.h"
#include "renderer/pipeline/modulators/signals/ScalarSignals.h"
#include "renderer/pipeline/modulators/angular/BoundedAngularModulator.h"
#include "renderer/pipeline/modulators/scalar/BoundedScalarModulator.h"

namespace PolarShader::UnitsTest {
    inline bool anglePromotionTest() {
        const AngleUnitsQ0_16 angles[] = {
            AngleUnitsQ0_16(0),
            AngleUnitsQ0_16(QUARTER_TURN_U16),
            AngleUnitsQ0_16(HALF_TURN_U16),
            AngleUnitsQ0_16(static_cast<uint16_t>(HALF_TURN_U16 + QUARTER_TURN_U16))
        };
        for (const auto &a: angles) {
            AngleTurnsUQ16_16 p = unbindAngle(a);
            AngleUnitsQ0_16 roundtrip = bindAngle(p);
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
            AngleTurnsUQ16_16 p = unbindAngle(a);
            int16_t direct = sin16(toFastLedPhase(p));
            int16_t wrapped = raw(sinQ1_15(p));
            if (direct != wrapped) return false;
        }
        return true;
    }

    inline bool phaseAccumulatorSmoothnessTest() {
        BoundedAngularModulator motion(AngleUnitsQ0_16(0),
                             constant(bound(UnboundedScalar::fromRaw(Q16_16_ONE / 4),
                                                BoundedAngularModulator::SPEED_MIN,
                                                BoundedAngularModulator::SPEED_MAX)));
        motion.advanceFrame(0);
        motion.advanceFrame(16);
        BoundedAngle p1 = motion.getPhase();
        motion.advanceFrame(32);
        BoundedAngle p2 = motion.getPhase();
        return ((raw(p1) & 0xFFFFu) != 0u) || ((raw(p2) & 0xFFFFu) != 0u);
    }

    inline bool phaseVelocityUnitTest() {
        BoundedAngularModulator motion(AngleUnitsQ0_16(0),
                             constant(bound(UnboundedScalar::fromRaw(Q16_16_ONE),
                                                BoundedAngularModulator::SPEED_MIN,
                                                BoundedAngularModulator::SPEED_MAX))); // 1 turn/sec
        const TimeMillis samples[] = {0, 200, 400, 600, 800, 1000};
        for (TimeMillis t: samples) {
            motion.advanceFrame(t);
        }
        return raw(motion.getPhase()) == raw(bindAngle(ANGLE_TURNS_ONE_TURN));
    }

    inline bool linearMotionAxisTest() {
        UnboundedScalar speed_value = UnboundedScalar::fromRaw(Q16_16_ONE); // 1 unit/sec
        auto speed = constant(bound(speed_value,
                                        BoundedScalarModulator::SPEED_MIN,
                                        BoundedScalarModulator::SPEED_MAX));
        auto direction = constant(BoundedAngle(0));
        BoundedScalarModulator motion(BoundedScalar(0), BoundedScalar(0), speed, direction);
        const TimeMillis samples[] = {0, 200, 400, 600, 800, 1000};
        for (TimeMillis t: samples) {
            motion.advanceFrame(t);
        }
        UnboundedScalar dt = millisToUnboundedScalar(200);
        UnboundedScalar step_distance = mul_q16_16_sat(speed_value, dt);
        int64_t step_dx = scale_q16_16_by_trig(RawQ16_16(step_distance.asRaw()),
                                               cosQ1_15(AngleTurnsUQ16_16(0)));
        uint16_t expected = static_cast<uint16_t>(step_dx * 5);
        return raw(motion.getRawX()) == expected && raw(motion.getRawY()) == 0;
    }

    inline bool linearMotionQuarterTurnTest() {
        UnboundedScalar speed_value = UnboundedScalar::fromRaw(Q16_16_ONE); // 1 unit/sec
        auto speed = constant(bound(speed_value,
                                        BoundedScalarModulator::SPEED_MIN,
                                        BoundedScalarModulator::SPEED_MAX));
        AngleUnitsQ0_16 quarter(QUARTER_TURN_U16);
        auto direction = constant(BoundedAngle(raw(quarter)));
        BoundedScalarModulator motion(BoundedScalar(0), BoundedScalar(0), speed, direction);
        const TimeMillis samples[] = {0, 200, 400, 600, 800, 1000};
        for (TimeMillis t: samples) {
            motion.advanceFrame(t);
        }
        AngleTurnsUQ16_16 phase = unbindAngle(quarter);
        UnboundedScalar dt = millisToUnboundedScalar(200);
        UnboundedScalar step_distance = mul_q16_16_sat(speed_value, dt);
        int64_t step_dy = scale_q16_16_by_trig(RawQ16_16(step_distance.asRaw()), sinQ1_15(phase));
        uint16_t expected_y = static_cast<uint16_t>(step_dy * 5);
        uint16_t x_raw = static_cast<uint16_t>(raw(motion.getRawX()));
        uint16_t x_abs = x_raw > (FRACT_Q0_16_MAX / 2)
                             ? static_cast<uint16_t>(FRACT_Q0_16_MAX - x_raw)
                             : x_raw;
        return x_abs <= 4 && raw(motion.getRawY()) == expected_y;
    }

    inline bool linearMotionQuantizationTest() {
        auto speed = constant(bound(UnboundedScalar::fromRaw(256),
                                        BoundedScalarModulator::SPEED_MIN,
                                        BoundedScalarModulator::SPEED_MAX)); // small but non-zero speed
        auto direction = constant(BoundedAngle(0));
        BoundedScalarModulator motion(BoundedScalar(0), BoundedScalar(0), speed, direction);
        TimeMillis t = 0;
        for (int i = 0; i < 100; ++i) {
            motion.advanceFrame(t);
            t += 16;
        }
        return raw(motion.getRawX()) != 0;
    }

    inline bool clampQ16RangeTest() {
        int64_t raw_value = static_cast<int64_t>(INT32_MAX) + 10;
        UnboundedScalar result = clamp_q16_16_raw(raw_value);
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
} // namespace PolarShader::UnitsTest
