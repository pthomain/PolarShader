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

#ifndef LED_SEGMENTS_TRANSFORMS_BASE_TRANSFORMS_H
#define LED_SEGMENTS_TRANSFORMS_BASE_TRANSFORMS_H

#include "Layers.h"
#include <polar/pipeline/utils/Units.h>

namespace LEDSegments {
    class FrameTransform {
    public:
        virtual ~FrameTransform() = default;

        virtual void advanceFrame(Units::TimeMillis timeInMillis) {};
    };

    class CartesianTransform : public FrameTransform {
    public:
        virtual CartesianLayer operator()(const CartesianLayer &layer) const = 0;
    };

    class PolarTransform : public FrameTransform {
    public:
        virtual PolarLayer operator()(const PolarLayer &layer) const = 0;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_BASE_TRANSFORMS_H
