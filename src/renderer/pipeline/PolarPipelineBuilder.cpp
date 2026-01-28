//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2025 Pierre Thomain

#include "PolarPipelineBuilder.h"
#include <Arduino.h>

namespace PolarShader {
    PolarPipeline PolarPipelineBuilder::build() {
        if (built) {
            Serial.println("PolarPipelineBuilder::build called more than once; returning black pipeline.");
            return PolarPipeline(std::move(pattern), palette, {}, name, context, depthSignal);
        }
        ensureFinalPolarDomain();
        built = true;
        return PolarPipeline(std::move(pattern), palette, std::move(steps), name, context, depthSignal);
    }
}
