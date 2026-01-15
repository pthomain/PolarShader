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

#ifndef LED_SEGMENTS_TRANSFORMS_VORTEXTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_VORTEXTRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "polar/engine/pipeline/mappers/Signal.h"

namespace LEDSegments {
    /**
     * @class VortexTransform
     * @brief Applies a radius-dependent angular offset (twist) to the polar coordinates.
     *
     * Expects PolarLayer with PhaseTurnsUQ16_16 phase. Multiplies vortex strength (Q16.16) by radius (Q0.16)
     * and adds the result modulo 2^32; large strengths can wrap. Use when you want radial twist; set bounds
     * on the BoundedSignal to avoid unexpected wrap artifacts.
     */
    class VortexTransform : public PolarTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        /**
         * @brief Constructs a new VortexTransform.
         * @param vortex A signal that provides the vortex strength over time.
         */
        explicit VortexTransform(BoundedSignal vortex);

        void advanceFrame(Units::TimeMillis timeInMillis) override;

        PolarLayer operator()(const PolarLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_VORTEXTRANSFORM_H
