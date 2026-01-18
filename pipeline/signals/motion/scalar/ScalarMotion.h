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

#ifndef LED_SEGMENTS_PIPELINE_SIGNALS_MOTION_SCALAR_H
#define  LED_SEGMENTS_PIPELINE_SIGNALS_MOTION_SCALAR_H

#include "polar/pipeline/signals/modulators/ScalarModulators.h"
#include "polar/pipeline/utils/Units.h"

namespace LEDSegments {
    class ScalarMotion {
    public:
        explicit ScalarMotion(ScalarModulator delta);

        void advanceFrame(TimeMillis timeInMillis);

        int32_t getValue() const { return static_cast<int32_t>(value.asRaw() >> 16); }

        RawQ16_16 getRawValue() const { return RawQ16_16(value.asRaw()); }

    private:
        FracQ16_16 value = FracQ16_16(0);
        ScalarModulator delta;
    };
}

#endif // LED_SEGMENTS_PIPELINE_SIGNALS_MOTION_SCALAR_H
