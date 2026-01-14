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

#ifndef LED_SEGMENTS_EFFECTS_TRANSFORMS_VORTEXTRANSFORM_H
#define LED_SEGMENTS_EFFECTS_TRANSFORMS_VORTEXTRANSFORM_H

#include "base/Transforms.h"
#include "polar/engine/pipeline/mappers/Signal.h"

namespace LEDSegments {
    /**
     * @class VortexTransform
     * @brief Applies a radius-dependent angular offset (twist) to the polar coordinates.
     */
    class VortexTransform : public PolarTransform {
        BoundedSignal vortexSignal;

    public:
        /**
         * @brief Constructs a new VortexTransform.
         * @param vortex A signal that provides the vortex strength over time.
         */
        explicit VortexTransform(BoundedSignal vortex) : vortexSignal(std::move(vortex)) {
        }

        void advanceFrame(unsigned long timeInMillis) override {
            vortexSignal.advanceFrame(timeInMillis);
        }

        PolarLayer operator()(const PolarLayer &layer) const override {
            return [this, layer](uint16_t angle_turns, fract16 radius, unsigned long timeInMillis) {
                // vortexSignal.getValue() returns strength in turns as a Q16.16 value.
                int32_t vortex_strength_q16_16 = vortexSignal.getValue();

                // Scale the strength by the radius to get the angular offset.
                // (Q16.16 * Q0.16) -> Q16.32, then shift to get Q16.16
                int32_t offset_q16_16 = scale_i32_by_f16(vortex_strength_q16_16, radius);

                // Add the integer part of the offset to the angle.
                uint16_t new_angle_turns = angle_turns + (offset_q16_16 >> 16);

                return layer(new_angle_turns, radius, timeInMillis);
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_TRANSFORMS_VORTEXTRANSFORM_H
