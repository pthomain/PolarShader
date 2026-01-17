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

#ifndef POLARSPEC_H
#define POLARSPEC_H

#include "config/PolarEffectConfig.h"
#include "config/PolarLayoutConfig.h"
#include "config/PolarOverlayConfig.h"
#include "config/PolarParamConfig.h"
#include "config/PolarTransitionConfig.h"
#include "engine/displayspec/DisplaySpec.h"
#include "engine/utils/Utils.h"
#include <functional>
#include "engine/render/renderable/Polar.h"
#include "polar/pipeline/utils/Units.h"

using namespace LEDSegments;

/**
 * @brief Defines the physical layout and properties of a polar-coordinate based LED display.
 *
 * This class implements the DisplaySpec interface to describe a display composed of
 * concentric rings of LEDs. It provides methods to get the number of segments (rings),
 * the size of each segment, and to map logical pixel indices to physical indices.
 * It also provides the core function to convert a pixel index to polar coordinates.
 */
class PolarSpec : public DisplaySpec {
public:
    static constexpr int LED_PIN = 9;
    static constexpr EOrder RGB_ORDER = GRB;

    explicit PolarSpec();

    /**
     * @brief Gets the total number of LEDs in the display.
     */
    uint16_t nbLeds() const override { return 241; }

    /**
     * @brief Gets the number of segments (concentric rings).
     */
    uint16_t nbSegments(uint16_t layoutId) const override;

    /**
     * @brief Gets the number of LEDs in a specific segment (ring).
     */
    uint16_t segmentSize(uint16_t layoutId, uint16_t segmentIndex) const override;

    /**
     * @brief Converts a global pixel index to polar coordinates (angle and radius).
     * This is a core function for the polar effects.
     */
    PolarCoords toPolarCoords(uint16_t pixelIndex) const override;

    /**
     * @brief Maps a logical pixel index within a segment to its physical, absolute index
     * in the LED strip.
     */
    void mapLeds(
        uint16_t layoutId,
        uint16_t segmentIndex,
        uint16_t pixelIndex,
        FracQ0_16 progress,
        const std::function<void(uint16_t)> &onLedMapped
    ) const override;

    ~PolarSpec() override = default;
};

#endif //POLARSPEC_H
