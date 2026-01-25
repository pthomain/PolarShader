//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#include "PolarPipelineBuilder.h"
#include <Arduino.h>

namespace PolarShader {
    PolarPipeline PolarPipelineBuilder::build() {
        if (built) {
            Serial.println("PolarPipelineBuilder::build called more than once; returning black pipeline.");
            return PolarPipeline(sourceLayer, palette, {}, name, context);
        }
        ensureFinalPolarDomain();
        built = true;
        return PolarPipeline(sourceLayer, palette, std::move(steps), name, context);
    }
}
