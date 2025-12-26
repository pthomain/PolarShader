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

#ifndef POLAR_LAYOUT_CONFIG_H
#define POLAR_LAYOUT_CONFIG_H

#include "engine/displayspec/config/LayoutConfig.h"

using namespace LEDSegments;

enum PolarLayout {
    WHOLE,
    SEGMENTED
};

static const std::set<uint16_t> polarLayoutIds = {
    {WHOLE, SEGMENTED}
};

static const std::map<uint16_t, String> polarLayoutNames = {
    {WHOLE, "WHOLE"},
    {SEGMENTED, "SEGMENTED"}
};

/**
 * @brief Selects a layout based on the renderable type, returning weighted options.
 * The weights determine the probability of a layout being chosen.
 * @param type The type of the renderable.
 * @return A vector of layouts and their weights.
 */
static WeightedLayouts polarLayoutSelector(RenderableType type) {
    return {
        {WHOLE, 1},
        {SEGMENTED, 0}
    };
}

#endif //POLAR_LAYOUT_CONFIG_H
