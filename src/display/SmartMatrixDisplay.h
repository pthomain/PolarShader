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

#ifndef POLARSHADER_SMARTMATRIXDISPLAY_H
#define POLARSHADER_SMARTMATRIXDISPLAY_H

#include "FastLED.h"
#include "MatrixDisplaySpec.h"
#include "renderer/PolarRenderer.h"

namespace PolarShader {
    class SmartMatrixDisplay {
        PolarRenderer renderer;
        CRGB *outputArray;
        uint8_t refreshRateInMillis;

    public:
        explicit SmartMatrixDisplay(
            MatrixDisplaySpec &spec,
            uint8_t brightness = 255,
            uint8_t refreshRateInMillis = 30
        );

        void loop();

        ~SmartMatrixDisplay();
    };
}

#endif // POLARSHADER_SMARTMATRIXDISPLAY_H
