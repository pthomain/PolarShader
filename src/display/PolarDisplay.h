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

#ifndef POLARSHADER_POLARDISPLAY_H
#define POLARSHADER_POLARDISPLAY_H

#include "FastLED.h"
#include <renderer/PolarRenderer.h>
#include <type_traits>
#include "PolarDisplaySpec.h"

namespace PolarShader {
    template<typename SPEC>
    class Display {
        static_assert(std::is_base_of<PolarDisplaySpec, SPEC>::value, "SPEC must derive from PolarDisplaySpec");
        PolarRenderer renderer;
        CRGB *outputArray;
        uint8_t refreshRateInMillis;

    public:
        explicit Display(
            PolarDisplaySpec &spec,
            uint8_t brightness = 20,
            uint8_t refreshRateInMillis = 30
        ) : renderer(spec.nbLeds(), [&spec](uint16_t pixelIndex) { return spec.toPolarCoords(pixelIndex); }),
            outputArray(new CRGB[spec.nbLeds()]),
            refreshRateInMillis(refreshRateInMillis) {
            CFastLED::addLeds<WS2812, SPEC::LED_PIN, SPEC::RGB_ORDER>(outputArray, spec.nbLeds())
                    .setCorrection(TypicalLEDStrip);

            FastLED.setBrightness(brightness);
            FastLED.clear(true);
            FastLED.show();
        }

        void loop() {
            EVERY_N_MILLISECONDS(refreshRateInMillis) {
                renderer.render(outputArray, millis());
                FastLED.show();
            }
        }

        ~Display() {
            delete[] outputArray;
        }
    };
} // namespace PolarShader

#endif //POLARSHADER_POLARDISPLAY_H
