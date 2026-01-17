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

#include <stdint.h>
#include "MathUtils.h"
#include "Units.h"
#include "NoiseUtils.h"
#include "polar/pipeline/signals/Modulation.h"

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
        auto velocity = Constant(FracQ16_16::fromRaw(Q16_16_ONE / 4)); // 0.25 turns/sec
        detail::PhaseAccumulator acc(std::move(velocity));
        acc.advance(0);
        AngleTurnsUQ16_16 p1 = acc.advance(16);
        AngleTurnsUQ16_16 p2 = acc.advance(32);
        return ((raw(p1) & 0xFFFFu) != 0u) || ((raw(p2) & 0xFFFFu) != 0u);
    }

    inline bool phaseVelocityUnitTest() {
        auto velocity = ConstantPhaseVelocity(FracQ16_16::fromRaw(Q16_16_ONE)); // 1 turn/sec
        detail::PhaseAccumulator acc(std::move(velocity));
        acc.advance(0);
        AngleTurnsUQ16_16 after_one_sec = acc.advance(1000); // 1 second later
        return raw(after_one_sec) == raw(ANGLE_TURNS_ONE_TURN);
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
               && clampQ16RangeTest()
               && wrapAddSignedTest()
               && noiseNormalizationTest();
    }
} // namespace LEDSegments::UnitsTest
