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

#ifndef LED_SEGMENTS_TRANSFORMS_ANISOTROPICSCALETRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_ANISOTROPICSCALETRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "polar/pipeline/signals/Signal.h"
#include "polar/pipeline/utils/MathUtils.h"

namespace LEDSegments {
    /**
     * Scales Cartesian coordinates independently on X and Y using Q16.16 scale factors
     * (LinearSignals). Wrap is modulo 2^32 to match other domain warps.
     *
     * Parameters: sx, sy (LinearSignals, Q16.16).
     * Recommended order: early in Cartesian chain; combine with shear/tiling as needed before polar conversion.
     */
    class AnisotropicScaleTransform : public CartesianTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        AnisotropicScaleTransform(LinearSignal sx, LinearSignal sy);

        void advanceFrame(Units::TimeMillis timeInMillis) override;

        CartesianLayer operator()(const CartesianLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_ANISOTROPICSCALETRANSFORM_H
