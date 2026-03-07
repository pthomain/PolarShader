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

#ifdef RP2040_ENABLED
#include <pico/sync.h>
#endif

namespace PolarShader {
    template<typename SPEC>
    class FastLedDisplay {
        static_assert(std::is_base_of<PolarDisplaySpec, SPEC>::value, "SPEC must derive from PolarDisplaySpec");
        PolarRenderer renderer;
        CRGB *outputArray;
        uint8_t refreshRateInMillis;
        fl::vector<uint8_t> freePinsForEntropy = {2, 3, 4, 5, 6, 7, 8, 9, 10};

#ifdef RP2040_ENABLED
        bool dualCore{false};
        semaphore_t startSem; // Core 0 releases to start Core 1's render
        semaphore_t doneSem; // Core 1 releases when its render is done
#endif

        void addEntropy() {
            static bool seedSet = false;
            uint8_t entropy = 0;

            if (!seedSet) {
                for (auto pin: freePinsForEntropy) {
                    pinMode(pin, INPUT);
                }
            }

            for (uint8_t i = 0; i < 16; i++) {
                entropy = (entropy << 1) | (analogRead(freePinsForEntropy[i % freePinsForEntropy.size()]) & 1);
            }

            if (seedSet) {
                random16_add_entropy(entropy);
            } else {
                random16_set_seed(entropy);
                seedSet = true;
            }
        }

    public:
        explicit FastLedDisplay(
            PolarDisplaySpec &spec,
            uint8_t brightness = 20,
            uint8_t refreshRateInMillis = 30,
            bool dualCore = false
        ) : renderer(spec.nbLeds(), [pSpec = &spec](uint16_t pixelIndex) { return pSpec->toPolarCoords(pixelIndex); }),
            outputArray(new CRGB[spec.nbLeds()]),
            refreshRateInMillis(refreshRateInMillis) {
            freePinsForEntropy.erase(
                fl::remove(freePinsForEntropy.begin(), freePinsForEntropy.end(), SPEC::LED_PIN),
                freePinsForEntropy.end()
            );

            addEntropy();

            CFastLED::addLeds<WS2812, SPEC::LED_PIN, SPEC::RGB_ORDER>(outputArray, spec.nbLeds())
                    .setCorrection(TypicalLEDStrip);

            FastLED.setBrightness(brightness);
            FastLED.clear(true);
            FastLED.show();

#ifdef RP2040_ENABLED
            this->dualCore = dualCore;
            if (dualCore) {
                sem_init(&startSem, 0, 1); // 0 initial permits: Core 1 starts blocked
                sem_init(&doneSem, 0, 1); // 0 initial permits: Core 0 starts blocked
            }
#endif
        }

        void loop() {
            EVERY_N_MILLISECONDS(refreshRateInMillis) {
#ifdef RP2040_ENABLED
                if (dualCore) {
                    renderer.prepareFrame(millis());
                    sem_release(&startSem); // wake Core 1
                    renderer.renderSlice(outputArray, 0, 2, 0); // even pixels
                    sem_acquire_blocking(&doneSem); // wait for Core 1
                } else {
                    renderer.render(outputArray, millis());
                }
#else
                renderer.render(outputArray, millis());
#endif
                FastLED.show();
            }
        }

#ifdef RP2040_ENABLED
        // Called from loop1() on Core 1. Blocks until Core 0 signals a new frame,
        // renders odd pixels, then signals completion.
        void core1Loop() {
            if (!dualCore) return;
            sem_acquire_blocking(&startSem); // wait for Core 0
            renderer.renderSlice(outputArray, 1, 2, 1); // odd pixels
            sem_release(&doneSem); // signal Core 0 done
        }
#endif

        ~FastLedDisplay() {
            delete[] outputArray;
        }
    };
} // namespace PolarShader

#endif //POLARSHADER_POLARDISPLAY_H
