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

#ifndef POLARSHADER_MAIN_RP2040_SHARED_H
#define POLARSHADER_MAIN_RP2040_SHARED_H

#include <Arduino.h>
#include <atomic>

#ifdef RP2040_ENABLED

#include "display/FastLedDisplay.h"

#ifndef POLAR_SHADER_RP2040_BRIGHTNESS
#define POLAR_SHADER_RP2040_BRIGHTNESS 255
#endif

#ifndef POLAR_SHADER_RP2040_REFRESH_MS
#define POLAR_SHADER_RP2040_REFRESH_MS 30
#endif

#ifndef POLAR_SHADER_RP2040_DUAL_CORE
#define POLAR_SHADER_RP2040_DUAL_CORE 1
#endif

namespace PolarShader {
    template<typename Spec>
    class Rp2040DisplayApp {
        using Display = FastLedDisplay<Spec>;
        inline static std::atomic<Display *> display{nullptr};

    public:
        static void setup() {
            static Spec specInstance;
            Serial.begin(115200);
            Display *createdDisplay = new Display(
                specInstance,
                POLAR_SHADER_RP2040_BRIGHTNESS,
                POLAR_SHADER_RP2040_REFRESH_MS,
                POLAR_SHADER_RP2040_DUAL_CORE != 0
            );
            display.store(createdDisplay, std::memory_order_release);
        }

        static void setup1() {
            // Required for arduino-pico to launch Core 1.
        }

        static void loop() {
            Display *currentDisplay = display.load(std::memory_order_acquire);
            if (currentDisplay) {
                currentDisplay->loop();
            }
        }

        static void loop1() {
            Display *currentDisplay = display.load(std::memory_order_acquire);
            if (currentDisplay) {
                currentDisplay->core1Loop();
            }
        }
    };
}

#endif

#endif // POLARSHADER_MAIN_RP2040_SHARED_H
