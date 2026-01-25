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

#include "Presets.h"
#include <renderer/pipeline/CartesianNoiseLayers.h>
#include <renderer/pipeline/maths/Maths.h>
#include <renderer/pipeline/signals/Signals.h>
#include "renderer/pipeline/PolarPipelineBuilder.h"
#include "renderer/pipeline/transforms/cartesian/ZoomTransform.h"

namespace PolarShader {
    namespace {
        PolarPipeline buildSimplePreset(const CRGBPalette16 &palette, const char *name) {
            return PolarPipelineBuilder(noiseLayer, palette, name)
                    .addCartesianTransform(ZoomTransform(constant(frac(2))))
                    .build();
        }
    } // namespace

    PolarPipeline buildDefaultPreset(const CRGBPalette16 &palette) {
        return buildSimplePreset(palette, "Default");
    }

    PolarPipeline buildBarrelTunnelPreset(const CRGBPalette16 &palette) {
        return buildSimplePreset(palette, "Barrel Tunnel");
    }

    PolarPipeline buildNoiseWarpFlamePreset(const CRGBPalette16 &palette) {
        return buildSimplePreset(palette, "Noise Warp Flame");
    }

    PolarPipeline buildTiledMirrorMandalaPreset(const CRGBPalette16 &palette) {
        return buildSimplePreset(palette, "Tiled Mirror Mandala");
    }

    PolarPipeline buildLiquidMarblePreset(const CRGBPalette16 &palette) {
        return buildSimplePreset(palette, "Liquid Marble");
    }

    PolarPipeline buildHeatShimmerPreset(const CRGBPalette16 &palette) {
        return buildSimplePreset(palette, "Heat Shimmer");
    }

    PolarPipeline buildCRTRipplePreset(const CRGBPalette16 &palette) {
        return buildSimplePreset(palette, "CRT Ripple");
    }

    PolarPipeline buildSpiralGalaxyPreset(const CRGBPalette16 &palette) {
        return buildSimplePreset(palette, "Spiral Galaxy");
    }

    PolarPipeline buildElectricTunnelPreset(const CRGBPalette16 &palette) {
        return buildSimplePreset(palette, "Electric Tunnel");
    }

    PolarPipeline buildStarburstPulsePreset(const CRGBPalette16 &palette) {
        return buildSimplePreset(palette, "Starburst Pulse");
    }

    PolarPipeline buildRainShearPreset(const CRGBPalette16 &palette) {
        return buildSimplePreset(palette, "Rain Shear");
    }

    PolarPipeline buildRippleRingPreset(const CRGBPalette16 &palette) {
        return buildSimplePreset(palette, "Ripple Ring");
    }

    PolarPipeline buildFractalTileBloomPreset(const CRGBPalette16 &palette) {
        return buildSimplePreset(palette, "Fractal Tile Bloom");
    }

    PolarPipeline buildStutterPulsePreset(const CRGBPalette16 &palette) {
        return buildSimplePreset(palette, "Stutter Pulse");
    }

    PolarPipeline buildCurlFlowSmokePreset(const CRGBPalette16 &palette) {
        return buildSimplePreset(palette, "Curl Flow Smoke");
    }

    PolarPipeline buildPerspectiveDepthPreset(const CRGBPalette16 &palette) {
        return buildSimplePreset(palette, "Perspective Depth");
    }

    PolarPipeline buildPosterizedRingsPreset(const CRGBPalette16 &palette) {
        return buildSimplePreset(palette, "Posterized Rings");
    }

    PolarPipeline buildJitteredTilesPreset(const CRGBPalette16 &palette) {
        return buildSimplePreset(palette, "Jittered Tiles");
    }
}
