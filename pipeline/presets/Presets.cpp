//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2023 Pierre Thomain

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

#include "Presets.h"
#include <polar/pipeline/CartesianNoiseLayers.h>
#include <polar/pipeline/signals/modulators/AngularModulators.h>
#include <polar/pipeline/signals/modulators/ScalarModulators.h>
#include <polar/pipeline/signals/motion/MotionBuilder.h>
#include <polar/pipeline/transforms/AnisotropicScaleTransform.h>
#include <polar/pipeline/transforms/BendTransform.h>
#include <polar/pipeline/transforms/CurlFlowTransform.h>
#include <polar/pipeline/transforms/DomainWarpTransform.h>
#include <polar/pipeline/transforms/KaleidoscopeTransform.h>
#include <polar/pipeline/transforms/LensDistortionTransform.h>
#include <polar/pipeline/transforms/MirrorTransform.h>
#include <polar/pipeline/transforms/NoiseWarpTransform.h>
#include <polar/pipeline/transforms/PerspectiveWarpTransform.h>
#include <polar/pipeline/transforms/PosterizePolarTransform.h>
#include <polar/pipeline/transforms/RadialScaleTransform.h>
#include <polar/pipeline/transforms/RotationTransform.h>
#include <polar/pipeline/transforms/ShearTransform.h>
#include <polar/pipeline/transforms/TileJitterTransform.h>
#include <polar/pipeline/transforms/TilingTransform.h>
#include <polar/pipeline/transforms/TranslationTransform.h>
#include <polar/pipeline/transforms/VortexTransform.h>
#include <polar/pipeline/transforms/ZoomTransform.h>
#include "polar/pipeline/PolarPipelineBuilder.h"
#include "polar/pipeline/utils/MathUtils.h"

namespace LEDSegments {
    PolarPipeline buildDefaultPreset(const CRGBPalette16 &palette) {
        return PolarPipelineBuilder(noiseLayer, palette, "Default")
                // Fixed zoom at mid-range for a stable baseline.
                .addCartesianTransform(ZoomTransform(scalar(Constant(UINT16_MAX / 64))))
                // Slow constant translation of the noise domain (diagonal drift).
                .addCartesianTransform(TranslationTransform(
                    linearUnbounded(0,
                                    0,
                                    Constant(q16_frac(1, 2)),
                                    ConstantAngleTurns(angleTurns_frac(1, 2)))))
                .build();
    }

    PolarPipeline buildBarrelTunnelPreset(const CRGBPalette16 &palette) {
        // Mild domain warp + barrel distortion + kaleidoscope symmetry.
        return PolarPipelineBuilder(fBmLayer, palette, "BarrelTunnel")
                // Low-frequency domain drift
                .addCartesianTransform(
                    DomainWarpTransform(
                        linearUnbounded(0,
                                        0,
                                        Constant(q16_frac(1, 4)),
                                        ConstantAngleUnits(AngleUnitsQ0_16(800)))
                    )
                )
                // Barrel distortion
                .addPolarTransform(LensDistortionTransform(
                    scalar(Constant(q16_frac(1, 64))))) // subtle barrel
                // Gentle outward zoom
                .addPolarTransform(RadialScaleTransform(
                    scalar(Constant(q16_frac(1, 32))))) // subtle outward zoom
                // Angular symmetry
                .addPolarTransform(KaleidoscopeTransform(5, false, true))
                .build();
    }

    PolarPipeline buildNoiseWarpFlamePreset(const CRGBPalette16 &palette) {
        // Noise-driven warp, anisotropic stretch, gentle bend, and barrel distortion.
        return PolarPipelineBuilder(fBmLayer, palette, "NoiseWarpFlame")
                // Turbulent warp
                .addCartesianTransform(
                    NoiseWarpTransform(scalar(Constant(q16_frac(1, 20))),
                                       scalar(Constant(q16_frac(3, 100)))))
                // Stretch vertically
                .addCartesianTransform(AnisotropicScaleTransform(
                    scalar(Constant(FracQ16_16(1))),
                    scalar(Constant(FracQ16_16(2)))))
                // stretch Y
                // Curve along X
                .addCartesianTransform(BendTransform(scalar(Constant(q16_frac(1, 100))),
                                                     scalar(Constant(FracQ16_16(0)))))
                // gentle curve along X
                // Slight barrel
                .addPolarTransform(LensDistortionTransform(
                    scalar(Constant(q16_frac(1, 128))))) // slight barrel
                // Slow twist
                .addPolarTransform(VortexTransform(
                    scalar(Constant(q16_frac(1, 96)))))
                .build();
    }

    PolarPipeline buildTiledMirrorMandalaPreset(const CRGBPalette16 &palette) {
        // Tile, mirror, then fold into a mandala with rotation.
        return PolarPipelineBuilder(noiseLayer, palette, "TiledMirrorMandala")
                // Tile domain
                .addCartesianTransform(TilingTransform(1u << 18, 1u << 18)) // tile at moderate scale
                // Slow drift to keep features moving
                .addCartesianTransform(TranslationTransform(
                    linearUnbounded(0,
                                    0,
                                    Constant(q16_frac(1, 2)),
                                    ConstantAngleUnits(AngleUnitsQ0_16(900)))))
                // Mirror X for symmetry
                .addCartesianTransform(MirrorTransform(true, false))
                // Slow rotation
                .addPolarTransform(RotationTransform(
                    angular(AngleUnitsQ0_16(0), ConstantPhaseVelocity(q16_frac(1, 2)))))
                // Mandala folding
                .addPolarTransform(KaleidoscopeTransform(6, true, true))
                .build();
    }

    PolarPipeline buildLiquidMarblePreset(const CRGBPalette16 &palette) {
        // Flowing marble: slow noise warp + shear + anisotropic stretch + gentle vortex.
        return PolarPipelineBuilder(noiseLayer, palette, "LiquidMarble")
                // Low-amplitude warp
                .addCartesianTransform(NoiseWarpTransform(scalar(Constant(q16_frac(1, 24))),
                                                          scalar(Constant(q16_frac(1, 32)))))
                // Skew the domain
                .addCartesianTransform(ShearTransform(scalar(Constant(q16_frac(1, 12))),
                                                      scalar(Constant(q16_frac(-1, 16)))))
                // Compress Y
                .addCartesianTransform(AnisotropicScaleTransform(
                    scalar(Constant(FracQ16_16(1))),
                    scalar(Constant(q16_frac(3, 5)))))
                // stretch Y ~0.6
                // Gentle twist
                .addPolarTransform(VortexTransform(
                    scalar(Constant(q16_frac(1, 64)))))
                // Symmetry
                .addPolarTransform(KaleidoscopeTransform(4, false, true))
                .build();
    }

    PolarPipeline buildHeatShimmerPreset(const CRGBPalette16 &palette) {
        // Subtle bend + lens distortion + slow rotation for heat shimmer.
        return PolarPipelineBuilder(fBmLayer, palette, "HeatShimmer")
                // Slight curvature
                .addCartesianTransform(BendTransform(scalar(Constant(q16_frac(1, 128))),
                                                     scalar(Constant(FracQ16_16(0)))))
                // Light barrel
                .addPolarTransform(LensDistortionTransform(
                    scalar(Constant(q16_frac(1, 96))))) // slight barrel
                // Slow spin
                .addPolarTransform(RotationTransform(
                    angular(AngleUnitsQ0_16(0), ConstantPhaseVelocity(q16_frac(1, 16)))))
                .build();
    }

    PolarPipeline buildCRTRipplePreset(const CRGBPalette16 &palette) {
        // Tiled noise with noise warp and oscillating radial scale for CRT-like screen wobble.
        return PolarPipelineBuilder(noiseLayer, palette, "CRTRipple")
                // Tile domain
                .addCartesianTransform(TilingTransform(1u << 17, 1u << 17))
                // Subtle warp
                .addCartesianTransform(NoiseWarpTransform(scalar(Constant(q16_frac(1, 32))),
                                                          scalar(Constant(q16_frac(1, 48)))))
                // Breathing zoom
                .addPolarTransform(RadialScaleTransform(
                    scalar(Sine(
                        ConstantPhaseVelocity(q16_frac(1, 6)),
                        ConstantRawQ16_16(RawQ16_16(500))))))
                // oscillating zoom, raw 500 is tiny fraction
                // Mild symmetry
                .addPolarTransform(KaleidoscopeTransform(3, false, false))
                .build();
    }

    PolarPipeline buildSpiralGalaxyPreset(const CRGBPalette16 &palette) {
        // Slanted arms with vortex and mandala symmetry.
        return PolarPipelineBuilder(fBmLayer, palette, "SpiralGalaxy")
                // Slant arms
                .addCartesianTransform(ShearTransform(scalar(Constant(q16_frac(1, 8))),
                                                      scalar(Constant(FracQ16_16(0)))))
                // Strong twist
                .addPolarTransform(VortexTransform(
                    scalar(Constant(q16_frac(1, 24)))))
                // Pincushion
                .addPolarTransform(LensDistortionTransform(
                    scalar(Constant(q16_frac(-1, 80))))) // mild pincushion
                // Slow spin
                .addPolarTransform(RotationTransform(
                    angular(AngleUnitsQ0_16(0), ConstantPhaseVelocity(q16_frac(1, 8)))))
                // Mandala fold
                .addPolarTransform(KaleidoscopeTransform(4, true, true))
                .build();
    }

    PolarPipeline buildElectricTunnelPreset(const CRGBPalette16 &palette) {
        // Strong stretch, warp, radial push, fast rotation for a tunnel vibe.
        return PolarPipelineBuilder(fBmLayer, palette, "ElectricTunnel")
                // Heavy Y stretch
                .addCartesianTransform(AnisotropicScaleTransform(
                    scalar(Constant(FracQ16_16(1))),
                    scalar(Constant(FracQ16_16(3)))))
                // heavy Y stretch
                // Intense warp
                .addCartesianTransform(NoiseWarpTransform(scalar(Constant(q16_frac(1, 16))),
                                                          scalar(Constant(q16_frac(1, 20)))))
                // Outward push
                .addPolarTransform(RadialScaleTransform(
                    scalar(Constant(q16_frac(1, 12)))))
                // Fast rotation
                .addPolarTransform(RotationTransform(
                    angular(AngleUnitsQ0_16(0), ConstantPhaseVelocity(q16_frac(3, 2)))))
                // Mandala symmetry
                .addPolarTransform(KaleidoscopeTransform(5, true, true))
                .build();
    }

    PolarPipeline buildStarburstPulsePreset(const CRGBPalette16 &palette) {
        // Pulsing radial scale with kaleidoscope and barrel distortion.
        return PolarPipelineBuilder(noiseLayer, palette, "StarburstPulse")
                // Pulsed radial zoom
                .addPolarTransform(RadialScaleTransform(
                    scalar(Pulse(
                        ConstantPhaseVelocity(q16_frac(1, 5)),
                        ConstantRawQ16_16(RawQ16_16(2000))))))
                // Angular symmetry
                .addPolarTransform(KaleidoscopeTransform(6, false, true))
                // Barrel pop
                .addPolarTransform(LensDistortionTransform(
                    scalar(Constant(q16_frac(1, 72)))))
                .build();
    }

    PolarPipeline buildRainShearPreset(const CRGBPalette16 &palette) {
        // Strong shear/stretch to create slanted rain bands with subtle warp.
        return PolarPipelineBuilder(noiseLayer, palette, "RainShear")
                // Heavy shear
                .addCartesianTransform(ShearTransform(scalar(Constant(q16_frac(1, 6))),
                                                      scalar(Constant(FracQ16_16(0)))))
                // heavy X shear
                // Tall stretch
                .addCartesianTransform(AnisotropicScaleTransform(
                    scalar(Constant(FracQ16_16(1))),
                    scalar(Constant(FracQ16_16(4)))))
                // tall Y
                // Light warp
                .addCartesianTransform(NoiseWarpTransform(scalar(Constant(q16_frac(1, 40))),
                                                          scalar(Constant(q16_frac(1, 64)))))
                // Gentle lens
                .addPolarTransform(LensDistortionTransform(
                    scalar(Constant(q16_frac(1, 96)))))
                .build();
    }

    PolarPipeline buildRippleRingPreset(const CRGBPalette16 &palette) {
        // Sinusoidal radial scale with symmetry and slow rotation.
        return PolarPipelineBuilder(fBmLayer, palette, "RippleRing")
                // Oscillating radial scale
                .addPolarTransform(RadialScaleTransform(
                    scalar(Sine(
                        ConstantPhaseVelocity(q16_frac(1, 8)),
                        ConstantRawQ16_16(RawQ16_16(1500))))))
                // Mirror both axes (Cartesian symmetry before polar conversion)
                .addCartesianTransform(MirrorTransform(true, true))
                // Many petals
                .addPolarTransform(KaleidoscopeTransform(8, false, false))
                // Slow spin
                .addPolarTransform(RotationTransform(
                    angular(AngleUnitsQ0_16(0), ConstantPhaseVelocity(q16_frac(1, 12)))))
                .build();
    }

    PolarPipeline buildFractalTileBloomPreset(const CRGBPalette16 &palette) {
        // Tiled fBm with mirror and mandala folding for blooming symmetry.
        return PolarPipelineBuilder(fBmLayer, palette, "FractalTileBloom")
                // Tile domain
                .addCartesianTransform(TilingTransform(1u << 17, 1u << 17))
                // Break up with warp
                .addCartesianTransform(NoiseWarpTransform(scalar(Constant(q16_frac(1, 32))),
                                                          scalar(Constant(q16_frac(1, 32)))))
                // Mirror X
                .addCartesianTransform(MirrorTransform(true, false))
                // Small bend
                .addCartesianTransform(BendTransform(scalar(Constant(q16_frac(1, 96))),
                                                     scalar(Constant(FracQ16_16(0)))))
                // Mandala
                .addPolarTransform(KaleidoscopeTransform(6, true, true))
                // Pincushion
                .addPolarTransform(LensDistortionTransform(
                    scalar(Constant(q16_frac(-1, 96)))))
                .build();
    }

    PolarPipeline buildStutterPulsePreset(const CRGBPalette16 &palette) {
        // Pulsing radial scale with medium rotation and kaleidoscope; stutter comes from pulsed radius.
        return PolarPipelineBuilder(noiseLayer, palette, "StutterPulse")
                // Pulsed radius
                .addPolarTransform(RadialScaleTransform(
                    scalar(Pulse(
                        ConstantPhaseVelocity(q16_frac(1, 4)),
                        ConstantRawQ16_16(RawQ16_16(2500))))))
                // Medium rotation
                .addPolarTransform(RotationTransform(
                    angular(AngleUnitsQ0_16(0), ConstantPhaseVelocity(q16_frac(1, 2)))))
                // Symmetry
                .addPolarTransform(KaleidoscopeTransform(5, false, true))
                // Lens
                .addPolarTransform(LensDistortionTransform(
                    scalar(Constant(q16_frac(1, 80)))))
                .build();
    }

    PolarPipeline buildCurlFlowSmokePreset(const CRGBPalette16 &palette) {
        // Divergence-free curl flow for fluid smoke-like advection with light vortex and lens.
        return PolarPipelineBuilder(fBmLayer, palette, "CurlFlowSmoke")
                // Curl advection
                .addCartesianTransform(CurlFlowTransform(
                    scalar(Constant(q16_frac(1, 24))), 3))
                // Additional warp
                .addCartesianTransform(NoiseWarpTransform(scalar(Constant(q16_frac(1, 40))),
                                                          scalar(Constant(q16_frac(1, 40)))))
                // Twist
                .addPolarTransform(VortexTransform(
                    scalar(Constant(q16_frac(1, 80)))))
                // Lens
                .addPolarTransform(LensDistortionTransform(
                    scalar(Constant(q16_frac(1, 96)))))
                .build();
    }

    PolarPipeline buildPerspectiveDepthPreset(const CRGBPalette16 &palette) {
        // Perspective warp to fake depth, then kaleidoscope symmetry.
        return PolarPipelineBuilder(noiseLayer, palette, "PerspectiveDepth")
                // Perspective scale
                .addCartesianTransform(PerspectiveWarpTransform(
                    scalar(Constant(q16_frac(1, 12)))))
                // Slight shear
                .addCartesianTransform(ShearTransform(scalar(Constant(q16_frac(1, 14))),
                                                      scalar(Constant(FracQ16_16(0)))))
                // Symmetry
                .addPolarTransform(KaleidoscopeTransform(4, false, true))
                // Slow spin
                .addPolarTransform(RotationTransform(
                    angular(AngleUnitsQ0_16(0), ConstantPhaseVelocity(q16_frac(1, 10)))))
                .build();
    }

    PolarPipeline buildPosterizedRingsPreset(const CRGBPalette16 &palette) {
        // Quantized rings/sectors feeding into twist and rotation.
        return PolarPipelineBuilder(fBmLayer, palette, "PosterizedRings")
                // Quantize angle/radius
                .addPolarTransform(PosterizePolarTransform(12, 6))
                // Twist
                .addPolarTransform(VortexTransform(
                    scalar(Constant(q16_frac(1, 40)))))
                // Rotation
                .addPolarTransform(RotationTransform(
                    angular(AngleUnitsQ0_16(0), ConstantPhaseVelocity(q16_frac(1, 6)))))
                .build();
    }

    PolarPipeline buildJitteredTilesPreset(const CRGBPalette16 &palette) {
        // Tiling with per-tile jitter to break repetition, then mandala.
        return PolarPipelineBuilder(noiseLayer, palette, "JitteredTiles")
                // Tile
                .addCartesianTransform(TilingTransform(1u << 17, 1u << 17))
                // Jitter tiles
                .addCartesianTransform(TileJitterTransform(
                    1u << 17,
                    1u << 17,
                    scalar(Constant(FracQ16_16(1024))) // constant amplitude via signal
                ))
                // Mandala fold
                .addPolarTransform(KaleidoscopeTransform(5, true, true))
                .build();
    }
}
