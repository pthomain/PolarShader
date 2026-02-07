//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#include "LayerBuilder.h"
#include <Arduino.h>

namespace PolarShader {
    Layer LayerBuilder::build() {
        if (built) {
            Serial.println("LayerBuilder::build called more than once; returning black layer.");
            return Layer(std::move(pattern), palette, {}, name, context, depthSignal);
        }
        built = true;
        return Layer(std::move(pattern), palette, std::move(steps), name, context, depthSignal);
    }
}
