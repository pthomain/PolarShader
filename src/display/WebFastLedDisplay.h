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

#ifndef POLARSHADER_WEBFASTLEDDISPLAY_H
#define POLARSHADER_WEBFASTLEDDISPLAY_H

#include <type_traits>
#include <vector>

// FastLED master moved screenmap.h from fl/ to fl/math/. Pre-3.10.4
// releases keep it under fl/, so prefer the new path with a fallback.
// TODO: drop the fl/screenmap.h fallback once we're back on a tagged
// release that has fl/math/screenmap.h.
#if __has_include(<fl/math/screenmap.h>)
#include <fl/math/screenmap.h>
#else
#include <fl/screenmap.h>
#endif

#include "FastLED.h"
#include "PolarDisplaySpec.h"
#include "WebDisplayGeometry.h"

namespace PolarShader {
    template<typename SPEC>
    class WebFastLedDisplay {
        static_assert(std::is_base_of<PolarDisplaySpec, SPEC>::value, "SPEC must derive from PolarDisplaySpec");

        PolarRenderer renderer;
        std::vector<CRGB> outputArray;
        fl::ScreenMap screenMap;
        uint8_t refreshRateInMillis;

        static fl::ScreenMap makeScreenMap(const WebDisplayGeometry &geometry) {
            fl::ScreenMap map(static_cast<fl::u32>(geometry.points.size()), geometry.diameter);
            for (std::size_t i = 0; i < geometry.points.size(); ++i) {
                map.set(
                    static_cast<uint16_t>(i),
                    fl::vec2f{geometry.points[i].x, geometry.points[i].y}
                );
            }
            return map;
        }

    public:
        explicit WebFastLedDisplay(
            SPEC &spec,
            const WebDisplayGeometry &geometry,
            uint8_t brightness = 255,
            uint8_t refreshRateInMillis = 30
        ) : renderer(spec.nbLeds(), [pSpec = &spec](uint16_t pixelIndex) { return pSpec->toPolarCoords(pixelIndex); }),
            outputArray(spec.nbLeds()),
            screenMap(makeScreenMap(geometry)),
            refreshRateInMillis(refreshRateInMillis) {
            FastLED.addLeds<WS2812, SPEC::LED_PIN, SPEC::RGB_ORDER>(
                outputArray.data(),
                static_cast<int>(outputArray.size())
            ).setCorrection(TypicalLEDStrip).setScreenMap(screenMap);

            FastLED.setBrightness(brightness);
            FastLED.clear(true);
            FastLED.show();
        }

        void loop() {
            EVERY_N_MILLISECONDS(refreshRateInMillis) {
                renderer.render(outputArray.data(), millis());
                FastLED.show();
            }
        }

        // Pass-through to PolarRenderer::replaceScene. Used by the composer
        // sketch to swap the active Scene live without rebuilding the display.
        void replaceScene(std::unique_ptr<Scene> scene) {
            renderer.replaceScene(std::move(scene), millis());
        }
    };
}

#endif // POLARSHADER_WEBFASTLEDDISPLAY_H
