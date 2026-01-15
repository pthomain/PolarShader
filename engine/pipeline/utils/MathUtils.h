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
    fl::i16 scale_i16_by_f16(fl::i16 value, Units::FractQ0_16 scale);
    fl::i32 scale_i32_by_f16(fl::i32 value, Units::FractQ0_16 scale);

    fl::i32 scale_i32_by_q16_16(fl::i32 value, Units::RawSignalQ16_16 scale);
    fl::i32 scale_q16_16_by_f16(fl::i32 value_q16_16, Units::FractQ0_16 scale_f16);

    Units::SignalQ16_16 mul_q16_16_sat(Units::SignalQ16_16 a, Units::SignalQ16_16 b);

    Units::SignalQ16_16 mul_q16_16_wrap(Units::SignalQ16_16 a, Units::SignalQ16_16 b);

    Units::RawSignalQ16_16 add_sat_q16_16(Units::RawSignalQ16_16 a, Units::RawSignalQ16_16 b);

    Units::RawSignalQ16_16 add_wrap_q16_16(Units::RawSignalQ16_16 a, Units::RawSignalQ16_16 b);

    uint16_t sqrt_u32(uint32_t value);

    Units::FractQ0_16 divide_u16_as_fract16(fl::u16 numerator, fl::u16 denominator);

    Units::AngleTurns16 atan2_turns_approx(int16_t y, int16_t x);

    /**
     * @brief Computes pow(base, exp) with guards for edge cases.
     * @param base The base, a Q0.16 fraction (Units::FractQ0_16).
     * @param exp The exponent, a Q16.16 fixed-point value (Units::SignalQ16_16).
     * @return base^exp, as a Q16.16 value, clamped to the [0, 1] range.
     */
    Units::SignalQ16_16 pow_f16_q16(Units::FractQ0_16 base, Units::SignalQ16_16 exp);
}
#endif //LED_SEGMENTS_PIPELINE_UTILS_MATHUTILS_H
