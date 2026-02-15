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

#ifdef SMARTMATRIX_ENABLED

#undef assert
#include "MatrixHardware_Teensy4_ShieldV5.h"
#include <SmartMatrix.h>
#include "Matrix128x128DisplaySpec.h"
#include "Matrix64x64DisplaySpec.h"

namespace PolarShader {
    // For now, we allocate buffers for the largest possible display (128x128).
    // The SmartMatrix library requires constants for buffer allocation.
    using DefaultAllocationSpec = Matrix128x128DisplaySpec;

    constexpr uint16_t kMatrixWidth = DefaultAllocationSpec::DISPLAY_WIDTH;
    constexpr uint16_t kScreenHeight = DefaultAllocationSpec::DISPLAY_HEIGHT;
    constexpr uint8_t kColorDepth = 24;
    constexpr uint8_t kDmaBufferRows = 8;
    constexpr uint8_t kPanelType = SM_PANELTYPE_HUB75_64ROW_MOD32SCAN;
    constexpr uint32_t kMatrixOptions = SM_HUB75_OPTIONS_C_SHAPE_STACKING;
    constexpr uint32_t kBackgroundOptions = SM_BACKGROUND_OPTIONS_NONE;

    SMARTMATRIX_ALLOCATE_BUFFERS(
        matrix,
        kMatrixWidth,
        kScreenHeight,
        kColorDepth,
        kDmaBufferRows,
        kPanelType,
        kMatrixOptions
    );

    SMARTMATRIX_ALLOCATE_BACKGROUND_LAYER(
        backgroundLayer,
        kMatrixWidth,
        kScreenHeight,
        24,
        kBackgroundOptions
    );

    SmartMatrixDisplay::SmartMatrixDisplay(
        MatrixDisplaySpec &spec,
        uint8_t brightness,
        uint8_t refreshRateInMillis
    ) : spec(spec),
        renderer(spec.nbLeds(), [&spec](uint16_t pixelIndex) { return spec.toPolarCoords(pixelIndex); }),
        outputArray(new CRGB[spec.nbLeds()]),
        refreshRateInMillis(refreshRateInMillis) {
        matrix.addLayer(&backgroundLayer);
        matrix.setRefreshRate(120);
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
            const uint16_t nbLeds = spec.nbLeds();
            const uint16_t mWidth = spec.matrixWidth();
            const uint16_t subsample = spec.subsample();

            for (uint16_t pixelIndex = 0; pixelIndex < nbLeds; ++pixelIndex) {
                const uint16_t sx = pixelIndex % mWidth;
                const uint16_t sy = pixelIndex / mWidth;
                const uint16_t dx = sx * subsample;
                const uint16_t dy = sy * subsample;
                const rgb24 c = rgb24(outputArray[pixelIndex].r, outputArray[pixelIndex].g, outputArray[pixelIndex].b);

                for (uint16_t oy = 0; oy < subsample; ++oy) {
                    for (uint16_t ox = 0; ox < subsample; ++ox) {
                        buffer[(dy + oy) * kMatrixWidth + dx + ox] = c;
                    }
                }
            }

            backgroundLayer.swapBuffers(false);
        }
    }

    SmartMatrixDisplay::~SmartMatrixDisplay() {
        delete[] outputArray;
    }
}

#endif
