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

#ifndef LED_SEGMENTS_EFFECTS_DECORATORS_VORTEXDECORATOR_H
#define LED_SEGMENTS_EFFECTS_DECORATORS_VORTEXDECORATOR_H

#include "base/Decorators.h"

namespace LEDSegments {
    /**
     * @class VortexDecorator
     * @brief A stateless Polar decorator that applies a radius-dependent angular offset (twist).
     *
     * The intensity of the vortex is controlled by a ValueMapper, allowing for
     * dynamic spiral effects.
     */
    class VortexDecorator : public PolarDecorator {
        AbsoluteValue anglePerRadiusMapper;

    public:
        explicit VortexDecorator(AbsoluteValue anglePerRadiusMapper)
            : anglePerRadiusMapper(std::move(anglePerRadiusMapper)) {
        }

        PolarLayer operator()(const PolarLayer &layer) const override {
            return [this, layer](uint16_t angle, fract16 radius, unsigned long timeInMillis) {
                // The mapper's output is treated as a fract16 for scaling.
                auto anglePerRadiusQ16 = (fract16) anglePerRadiusMapper(timeInMillis);
                uint16_t offset = scale_u16_by_f16(anglePerRadiusQ16, radius);
                uint16_t newAngle = angle + offset;
                return layer(newAngle, radius, timeInMillis);
            };
        }
    };
}

#endif //LED_SEGMENTS_EFFECTS_DECORATORS_VORTEXDECORATOR_H
