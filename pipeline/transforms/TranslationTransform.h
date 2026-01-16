//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

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

#ifndef LED_SEGMENTS_TRANSFORMS_TRANSLATIONTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_TRANSLATIONTRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "polar/pipeline/signals/Signal.h"

namespace LEDSegments {
    /**
     * Applies a simple Cartesian translation: (x, y) -> (x + dx, y + dy).
     * Offsets are provided as LinearSignals (Q16.16), integer part is used.
     *
     * Parameters: dx, dy (LinearSignal).
     * Recommended order: early in the Cartesian chain before warps/tiling.
     */
    class TranslationTransform : public CartesianTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        TranslationTransform(LinearSignal dx, LinearSignal dy);

        void advanceFrame(Units::TimeMillis timeInMillis) override;

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_TRANSLATIONTRANSFORM_H
