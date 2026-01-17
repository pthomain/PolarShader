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

#ifndef LED_SEGMENTS_PIPELINE_UTILS_MATHUTILS_H
#define LED_SEGMENTS_PIPELINE_UTILS_MATHUTILS_H

#include "Units.h"

namespace LEDSegments {
    fl::i16 scale_i16_by_f16(fl::i16 value, FracQ0_16 scale);

    fl::i32 scale_i32_by_f16(fl::i32 value, FracQ0_16 scale);

    fl::i32 scale_i32_by_q16_16(fl::i32 value, RawQ16_16 scale);

    fl::i32 scale_q16_16_by_f16(fl::i32 value_q16_16, FracQ0_16 scale_f16);

    FracQ16_16 mul_q16_16_sat(FracQ16_16 a, FracQ16_16 b);

    FracQ16_16 mul_q16_16_wrap(FracQ16_16 a, FracQ16_16 b);

    RawQ16_16 add_sat_q16_16(RawQ16_16 a, RawQ16_16 b);

    RawQ16_16 add_wrap_q16_16(RawQ16_16 a, RawQ16_16 b);

    uint16_t sqrt_u32(uint32_t value);

    FracQ0_16 divide_u16_as_fract16(fl::u16 numerator, fl::u16 denominator);

    AngleUnitsQ0_16 atan2_turns_approx(int16_t y, int16_t x);

    /**
     * @brief Retention-focused pow(base, exp) with clamps.
     * Assumes exp >= 0 (typical for damping over time) and clamps the result to [0,1].
     * @param base The base, a Q0.16 fraction (FracQ0_16).
     * @param exp The exponent, a Q16.16 fixed-point value (FracQ16_16).
     * @return base^exp, as a Q16.16 value, clamped to the [0, 1] range.
     */
    FracQ16_16 pow_f16_q16(FracQ0_16 base, FracQ16_16 exp);

    uint64_t sqrt_u64(uint64_t value);

    int64_t scale_q16_16_by_trig(RawQ16_16 magnitude, TrigQ1_15 trig_q1_15);

    int16_t clamp_raw_to_i16(RawQ16_16 raw_value);

    int64_t clamp_raw(int64_t value);

    inline FracQ16_16 mulTrigQ1_15_SignalQ16_16(TrigQ1_15 t, FracQ16_16 amp) {
        int64_t scaled = static_cast<int64_t>(raw(t)) * amp.asRaw(); // Q1.15 * Q16.16 = Q17.31
        int64_t shifted = scaled >> 15;
        int64_t clamped = clamp_raw(shifted);
        return FracQ16_16::fromRaw(static_cast<int32_t>(clamped));
    }
}
#endif //LED_SEGMENTS_PIPELINE_UTILS_MATHUTILS_H
