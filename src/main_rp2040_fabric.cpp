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

#include "FabricDisplaySpec.h"
#include "main_rp2040_shared.h"

#ifdef RP2040_ENABLED
extern "C" {
bool core1_separate_stack = true;
}

void setup() {
    PolarShader::Rp2040DisplayApp<PolarShader::FabricDisplaySpec>::setup();
}

void setup1() {
    PolarShader::Rp2040DisplayApp<PolarShader::FabricDisplaySpec>::setup1();
}

void loop() {
    PolarShader::Rp2040DisplayApp<PolarShader::FabricDisplaySpec>::loop();
}

void loop1() {
    PolarShader::Rp2040DisplayApp<PolarShader::FabricDisplaySpec>::loop1();
}
#endif
