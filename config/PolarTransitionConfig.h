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

#ifndef POLAR_TRANSITION_CONFIG_H
#define POLAR_TRANSITION_CONFIG_H

#include "engine/displayspec/config/LayoutConfig.h"
#include "transitions/fade/FadeTransition.h"

using namespace LEDSegments;

/**
 * @brief Selects the transition to be used between effects for a given layout.
 * @param layoutId The ID of the current layout.
 * @return A list of renderable transitions and their mirrors.
 *
 * This is currently hardcoded to always return the FadeTransition.
 */
static RenderablesAndMirrors<uint8_t> polarTransitionSelector(uint16_t layoutId) {
    return {
        just(FadeTransition::factory),
        noMirrors<uint8_t>
    };
}

#endif //POLAR_TRANSITION_CONFIG_H
