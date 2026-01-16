//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#include "PresetPicker.h"
#include <FastLED.h>

namespace LEDSegments {
    PolarPipeline PresetPicker::pickRandom(const CRGBPalette16 &palette) {
        static const PresetBuilder builders[] = {
            buildBarrelTunnelPreset,
            buildNoiseWarpFlamePreset,
            buildTiledMirrorMandalaPreset,
            buildLiquidMarblePreset,
            buildHeatShimmerPreset,
            buildCRTRipplePreset,
            buildSpiralGalaxyPreset,
            buildElectricTunnelPreset,
            buildStarburstPulsePreset,
            buildRainShearPreset,
            buildRippleRingPreset,
            buildFractalTileBloomPreset,
            buildStutterPulsePreset,
            buildCurlFlowSmokePreset,
            buildPerspectiveDepthPreset,
            buildPosterizedRingsPreset,
            buildJitteredTilesPreset,
            buildTimeStutterPreset
        };
        uint8_t idx = random8(sizeof(builders) / sizeof(builders[0]));
        return builders[idx](palette);
    }
}
