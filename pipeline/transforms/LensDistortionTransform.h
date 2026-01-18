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

#ifndef LED_SEGMENTS_TRANSFORMS_LENSDISTORTIONTRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_LENSDISTORTIONTRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "polar/pipeline/signals/motion/scalar/ScalarMotion.h"
#include "polar/pipeline/utils/Units.h"

namespace LEDSegments {
    /**
     * Barrel/pincushion lens distortion on polar radius: r' = r * (1 + k * r), clamped to [0..FRACT_MAX].
     * Positive k produces barrel (expands outer radii), negative k produces pincushion (pulls in outer radii).
     *
     * Parameters: k (ScalarMotion, Q16.16).
     * Recommended order: in polar chain before kaleidoscope/mandala so symmetry sees the distorted radius.
     */
    class LensDistortionTransform : public PolarTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        explicit LensDistortionTransform(ScalarMotion k);

        void advanceFrame(TimeMillis timeInMillis) override;

        PolarLayer operator()(const PolarLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_LENSDISTORTIONTRANSFORM_H
