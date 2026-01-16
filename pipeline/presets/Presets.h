//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

/*
 * This file is part of LED Segments.
 *
 * LED Segments is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LED Segments is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LED Segments. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LED_SEGMENTS_PIPELINE_PRESETS_H
#define LED_SEGMENTS_PIPELINE_PRESETS_H

#include "../PolarPipeline.h"

namespace LEDSegments {
    // Demonstration presets combining the new transforms. These are examples to seed effects;
    // tweak signals/parameters to taste.

    // Barrel/pincushion tunnel with mild domain warp and kaleidoscope symmetry.
    PolarPipeline buildBarrelTunnelPreset(const CRGBPalette16 &palette);

    // Flame-like noise with anisotropic stretch, noise-driven warp, and soft barrel distortion.
    PolarPipeline buildNoiseWarpFlamePreset(const CRGBPalette16 &palette);

    // Repeating mirrored tiles folded into a mandala.
    PolarPipeline buildTiledMirrorMandalaPreset(const CRGBPalette16 &palette);

    // Flowing marble veins with shear, anisotropic stretch, and gentle vortex.
    PolarPipeline buildLiquidMarblePreset(const CRGBPalette16 &palette);

    // Subtle heat shimmer: bend + lens distortion + slow rotation.
    PolarPipeline buildHeatShimmerPreset(const CRGBPalette16 &palette);

    // CRT-like ripple: tiled noise with noise warp and oscillating radial scale.
    PolarPipeline buildCRTRipplePreset(const CRGBPalette16 &palette);

    // Spiral galaxy: slanted arms, vortex twist, mandala symmetry.
    PolarPipeline buildSpiralGalaxyPreset(const CRGBPalette16 &palette);

    // Electric tunnel: strong stretch, warp, radial push, fast rotation.
    PolarPipeline buildElectricTunnelPreset(const CRGBPalette16 &palette);

    // Starburst pulse: pulsing radial scale, kaleidoscope, barrel distortion.
    PolarPipeline buildStarburstPulsePreset(const CRGBPalette16 &palette);

    // Rain shear: strong shear/stretch for rain bands with subtle warp.
    PolarPipeline buildRainShearPreset(const CRGBPalette16 &palette);

    // Ripple ring: sinusoidal radial scale with symmetry and slow rotation.
    PolarPipeline buildRippleRingPreset(const CRGBPalette16 &palette);

    // Fractal tile bloom: tiled fBm with mirror and mandala folding.
    PolarPipeline buildFractalTileBloomPreset(const CRGBPalette16 &palette);

    // Stutter pulse: pulsing radial scale with medium rotation and kaleidoscope.
    PolarPipeline buildStutterPulsePreset(const CRGBPalette16 &palette);

    // Curl flow smoke: divergence-free flow with gentle warp and vortex.
    PolarPipeline buildCurlFlowSmokePreset(const CRGBPalette16 &palette);

    // Perspective depth: perspective warp plus symmetry.
    PolarPipeline buildPerspectiveDepthPreset(const CRGBPalette16 &palette);

    // Posterized rings: quantized rings/sectors feeding into twist/rotation.
    PolarPipeline buildPosterizedRingsPreset(const CRGBPalette16 &palette);

    // Jittered tiles: tile with per-tile jitter and mandala folding.
    PolarPipeline buildJitteredTilesPreset(const CRGBPalette16 &palette);
}

#endif //LED_SEGMENTS_PIPELINE_PRESETS_H
