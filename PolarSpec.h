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
#include <functional> // Add this include
#include <utility>    // For std::pair

using namespace LEDSegments;

class PolarSpec : public DisplaySpec {
public:
    static constexpr int LED_PIN = 9;
    static constexpr EOrder RGB_ORDER = GRB;

    explicit PolarSpec() : DisplaySpec(
        LayoutConfig(
            polarLayoutIds,
            polarLayoutNames,
            polarLayoutSelector,
            polarEffectSelector,
            polarOverlaySelector,
            polarTransitionSelector,
            polarParamSelector
        ),
        DEBUG ? 40 : 128,
        DEBUG ? 3600 : 3,
        DEBUG ? 3600 : 8,
        1000,
        1.0f,
        30,
        true
    ) {
    }

    uint16_t nbLeds() const override { return 241; }

    uint16_t nbSegments(uint16_t layoutId) const override;

    uint16_t segmentSize(uint16_t layoutId, uint16_t segmentIndex) const override;

    void toPolarCoords(uint16_t pixelIndex, PolarContext& context) const override;

    void mapLeds(
        uint16_t layoutId,
        uint16_t segmentIndex,
        uint16_t pixelIndex,
        fract16 progress,
        const std::function<void(uint16_t)> &onLedMapped
    ) const override;

    ~PolarSpec() override = default;
};

#endif //POLARSPEC_H
