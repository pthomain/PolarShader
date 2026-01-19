//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

/*
 * This file is part of PolarShader.
 *
 * PolarShader is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * PolarShader is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PolarShader. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef POLARSHADER_POLARDISPLAYSPEC_H
#define POLARSHADER_POLARDISPLAYSPEC_H

#include "FastLED.h"
#include "renderer/PolarRenderer.h"

namespace PolarShader {
    class PolarDisplaySpec {
    public:
        virtual uint16_t numSegments() const = 0;

        virtual uint16_t nbLeds() const = 0;

        virtual uint16_t segmentSize(uint16_t segmentIndex) const = 0;

        virtual PolarCoords toPolarCoords(uint16_t pixelIndex) const = 0;

        virtual ~PolarDisplaySpec() = default;
    };
}
#endif //POLARSHADER_POLARDISPLAYSPEC_H
