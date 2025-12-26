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

#ifndef POLAR_PARAM_CONFIG_H
#define POLAR_PARAM_CONFIG_H

using namespace LEDSegments;

/**
 * @brief Selects a map of parameters for a given renderable.
 * @param type The type of the renderable (effect, overlay, etc.).
 * @param renderableId The ID of the specific renderable.
 * @param layoutId The ID of the current layout.
 * @param mirror The mirror configuration.
 * @return A map of parameter IDs to values.
 *
 * This is currently hardcoded to return an empty map, meaning no parameters are configured.
 */
static std::map<uint8_t, uint16_t> polarParamSelector(
    RenderableType type,
    TypeInfo::ID renderableId,
    uint16_t layoutId,
    Mirror mirror
) {
    return {};
}

#endif //POLAR_PARAM_CONFIG_H
