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

#include "FastLED.h"
#include "Units.h"

namespace LEDSegments {
    fl::i16 scale_i16_by_f16(fl::i16 value, Units::FracQ0_16 scale);

    fl::i32 scale_i32_by_f16(fl::i32 value, Units::FracQ0_16 scale);

    fl::i32 scale_i32_by_q16_16(fl::i32 value, Units::RawFracQ16_16 scale);

    fl::i32 scale_q16_16_by_f16(fl::i32 value_q16_16, Units::FracQ0_16 scale_f16);

    Units::FracQ16_16 mul_q16_16_sat(Units::FracQ16_16 a, Units::FracQ16_16 b);

    Units::FracQ16_16 mul_q16_16_wrap(Units::FracQ16_16 a, Units::FracQ16_16 b);

    Units::RawFracQ16_16 add_sat_q16_16(Units::RawFracQ16_16 a, Units::RawFracQ16_16 b);

    Units::RawFracQ16_16 add_wrap_q16_16(Units::RawFracQ16_16 a, Units::RawFracQ16_16 b);

    uint16_t sqrt_u32(uint32_t value);

    Units::FracQ0_16 divide_u16_as_fract16(fl::u16 numerator, fl::u16 denominator);

    Units::AngleUnitsQ0_16 atan2_turns_approx(int16_t y, int16_t x);

    /**
     * @brief Retention-focused pow(base, exp) with clamps.
     * Assumes exp >= 0 (typical for damping over time) and clamps the result to [0,1].
     * @param base The base, a Q0.16 fraction (Units::FracQ0_16).
     * @param exp The exponent, a Q16.16 fixed-point value (Units::FracQ16_16).
     * @return base^exp, as a Q16.16 value, clamped to the [0, 1] range.
     */
    Units::FracQ16_16 pow_f16_q16(Units::FracQ0_16 base, Units::FracQ16_16 exp);

    uint64_t sqrt_u64(uint64_t value);

    int64_t scale_q16_16_by_trig(Units::RawFracQ16_16 magnitude, int32_t trig_q1_15);

    int16_t clamp_raw_to_i16(Units::RawFracQ16_16 raw);

    int64_t clamp_raw(int64_t value);
}
#endif //LED_SEGMENTS_PIPELINE_UTILS_MATHUTILS_H
