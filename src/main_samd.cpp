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
#include "RoundDisplaySpec.h"
#include "display/FastLedDisplay.h"

using namespace PolarShader;
using PolarDisplay = FastLedDisplay<RoundDisplaySpec>;

static PolarDisplay *display = nullptr;

void setup() {
    static RoundDisplaySpec specInstance;
    Serial.begin(115200);
    while (!Serial && millis() < 2000) {
    }

    display = new PolarDisplay(specInstance, 30);
}

void loop() {
    // display->loop();
}
