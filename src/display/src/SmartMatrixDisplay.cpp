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

#include "display/SmartMatrixDisplay.h"
#undef assert
#include "MatrixHardware_Teensy4_ShieldV5.h"
#include <SmartMatrix.h>

namespace PolarShader {
    constexpr uint16_t kMatrixWidth = MatrixDisplaySpec::MATRIX_WIDTH;
    constexpr uint16_t kMatrixHeight = MatrixDisplaySpec::MATRIX_HEIGHT;
    constexpr uint8_t kColorDepth = 24;
    constexpr uint8_t kDmaBufferRows = 4;
    constexpr uint8_t kPanelType = SM_PANELTYPE_HUB75_64ROW_MOD32SCAN;
    constexpr uint32_t kMatrixOptions = SM_HUB75_OPTIONS_NONE;
    constexpr uint32_t kBackgroundOptions = SM_BACKGROUND_OPTIONS_NONE;

    SMARTMATRIX_ALLOCATE_BUFFERS(
        matrix,
        kMatrixWidth,
        kMatrixHeight,
        kColorDepth,
        kDmaBufferRows,
        kPanelType,
        kMatrixOptions
    );

    SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(
        backgroundLayer,
        kMatrixWidth,
        kMatrixHeight,
        24,
        kBackgroundOptions
    );

    SmartMatrixDisplay::SmartMatrixDisplay(
        MatrixDisplaySpec &spec,
        uint8_t brightness,
        uint8_t refreshRateInMillis
    ) : renderer(spec.nbLeds(), [&spec](uint16_t pixelIndex) { return spec.toPolarCoords(pixelIndex); }),
        outputArray(new CRGB[spec.nbLeds()]),
        refreshRateInMillis(refreshRateInMillis) {
        matrix.addLayer(&backgroundLayer);
        matrix.begin();
        backgroundLayer.setBrightness(brightness);
        backgroundLayer.enableColorCorrection(false);
        backgroundLayer.fillScreen({0, 0, 0});
        backgroundLayer.swapBuffers(true);
    }

    void SmartMatrixDisplay::loop() {
        EVERY_N_MILLISECONDS(refreshRateInMillis) {
            renderer.render(outputArray, millis());

            auto *buffer = backgroundLayer.backBuffer();

            for (uint16_t pixelIndex = 0; pixelIndex < MatrixDisplaySpec::NB_LEDS; ++pixelIndex) {
                uint16_t x = pixelIndex % MatrixDisplaySpec::MATRIX_WIDTH;
                uint16_t y = pixelIndex / MatrixDisplaySpec::MATRIX_WIDTH;
                const CRGB &color = outputArray[pixelIndex];
                buffer[(y * MatrixDisplaySpec::MATRIX_WIDTH) + x] = rgb24(color.r, color.g, color.b);
            }

            backgroundLayer.swapBuffers(false);
        }
    }

    SmartMatrixDisplay::~SmartMatrixDisplay() {
        delete[] outputArray;
    }
}
