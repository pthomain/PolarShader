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

#include <Arduino.h>
#include <atomic>

#ifdef RP2040_ENABLED

#include "FabricDisplaySpec.h"
#include "display/FastLedDisplay.h"

using namespace PolarShader;
using PolarDisplay = FastLedDisplay<FabricDisplaySpec>;

extern "C" {
bool core1_separate_stack = true;
}

static std::atomic<PolarDisplay *> display{nullptr};

void setup() {
    static FabricDisplaySpec specInstance;
    Serial.begin(115200);
    PolarDisplay *createdDisplay = new PolarDisplay(specInstance, 255, 30, true);
    display.store(createdDisplay, std::memory_order_release);
}

void setup1() { /* required for arduino-pico to launch Core 1 */ }

void loop() {
    PolarDisplay *currentDisplay = display.load(std::memory_order_acquire);
    if (currentDisplay) {
        currentDisplay->loop();
    }
}

void loop1() {
    PolarDisplay *currentDisplay = display.load(std::memory_order_acquire);
    if (currentDisplay) {
        currentDisplay->core1Loop();
    }
}

#endif
