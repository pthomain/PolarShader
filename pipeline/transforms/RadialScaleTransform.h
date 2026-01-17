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

#ifndef LED_SEGMENTS_TRANSFORMS_RADIALSCALETRANSFORM_H
#define LED_SEGMENTS_TRANSFORMS_RADIALSCALETRANSFORM_H

#include <memory>
#include "base/Transforms.h"
#include "polar/pipeline/signals/motion/Motion.h"

namespace LEDSegments {
    /**
     * Scales polar radius by a radial function: r' = r + (k * r) where k is a ScalarMotion (Q16.16).
     * Useful for tunnel/zoom effects. Output radius is clamped to [0, FRACT_Q0_16_MAX].
     *
     * Parameters: k (ScalarMotion, Q16.16 factor).
     * Recommended order: in polar chain before kaleidoscope/mandala; combines well with vortex/rotation.
     */
    class RadialScaleTransform : public PolarTransform {
        struct State;
        std::shared_ptr<State> state;

    public:
        explicit RadialScaleTransform(ScalarMotion k);

        void advanceFrame(Units::TimeMillis timeInMillis) override;

        PolarLayer operator()(const PolarLayer &layer) const override;
    };
}

#endif //LED_SEGMENTS_TRANSFORMS_RADIALSCALETRANSFORM_H
