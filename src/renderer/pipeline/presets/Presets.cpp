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
#include <renderer/pipeline/modulators/signals/AngularSignals.h>
#include <renderer/pipeline/modulators/signals/ScalarSignals.h>
#include <renderer/pipeline/modulators/angular/BoundedAngularModulator.h>
#include <renderer/pipeline/modulators/scalar/BoundedScalarModulator.h>
#include <renderer/pipeline/transforms/AnisotropicScaleTransform.h>
#include <renderer/pipeline/transforms/BendTransform.h>
#include <renderer/pipeline/transforms/CurlFlowTransform.h>
#include <renderer/pipeline/transforms/DomainWarpTransform.h>
#include <renderer/pipeline/transforms/KaleidoscopeTransform.h>
#include <renderer/pipeline/transforms/LensDistortionTransform.h>
#include <renderer/pipeline/transforms/MirrorTransform.h>
#include <renderer/pipeline/transforms/NoiseWarpTransform.h>
#include <renderer/pipeline/transforms/PerspectiveWarpTransform.h>
#include <renderer/pipeline/transforms/PosterizePolarTransform.h>
#include <renderer/pipeline/transforms/RadialScaleTransform.h>
#include <renderer/pipeline/transforms/RotationTransform.h>
#include <renderer/pipeline/transforms/ShearTransform.h>
#include <renderer/pipeline/transforms/TileJitterTransform.h>
#include <renderer/pipeline/transforms/TilingTransform.h>
#include <renderer/pipeline/transforms/TranslationTransform.h>
#include <renderer/pipeline/transforms/VortexTransform.h>
#include <renderer/pipeline/transforms/ZoomTransform.h>
#include <renderer/pipeline/modulators/BoundUtils.h>
#include "renderer/pipeline/PolarPipelineBuilder.h"

namespace PolarShader {
    PolarPipeline buildDefaultPreset(const CRGBPalette16 &palette) {
        return PolarPipelineBuilder(noiseLayer, palette, "Default")
                // Fixed zoom at mid-range for a stable baseline.
                .addCartesianTransform(ZoomTransform(
                    constant(boundedScalar(1, 2))))
                // Slow constant translation of the noise domain (diagonal drift).
                .addCartesianTransform(TranslationTransform(
                    BoundedScalarModulator(BoundedScalar(0),
                                           BoundedScalar(0),
                                           constant(bound(unboundedScalar(1, 2),
                                                          BoundedScalarModulator::SPEED_MIN,
                                                          BoundedScalarModulator::SPEED_MAX)),
                                           constant(bindAngle(unboundedAngle(1, 2))))))
                .build();
    }

    PolarPipeline buildBarrelTunnelPreset(const CRGBPalette16 &palette) {
        // Mild domain warp + barrel distortion + kaleidoscope symmetry.
        return PolarPipelineBuilder(fBmLayer, palette, "BarrelTunnel")
                // Low-frequency domain drift
                .addCartesianTransform(
                    DomainWarpTransform(
                        BoundedScalarModulator(BoundedScalar(0),
                                               BoundedScalar(0),
                                               constant(bound(unboundedScalar(1, 4),
                                                              BoundedScalarModulator::SPEED_MIN,
                                                              BoundedScalarModulator::SPEED_MAX)),
                                               constant(BoundedAngle(800)))
                    )
                )
                // Barrel distortion
                .addPolarTransform(LensDistortionTransform(
                    constant(boundedScalar(9, 16)))) // subtle barrel
                // Gentle outward zoom
                .addPolarTransform(RadialScaleTransform(
                    constant(boundedAngle(17, 32)))) // subtle outward zoom
                // Angular symmetry
                .addPolarTransform(KaleidoscopeTransform(5, false, true))
                .build();
    }

    PolarPipeline buildNoiseWarpFlamePreset(const CRGBPalette16 &palette) {
        // Noise-driven warp, anisotropic stretch, gentle bend, and barrel distortion.
        return PolarPipelineBuilder(fBmLayer, palette, "NoiseWarpFlame")
                // Turbulent warp
                .addCartesianTransform(
                    NoiseWarpTransform(
                        constant(boundedScalar(3, 5)),
                        constant(boundedScalar(14, 25))))
                // Stretch vertically
                .addCartesianTransform(AnisotropicScaleTransform(
                    constant(boundedScalar(1, 4)),
                    constant(boundedScalar(1, 2))))
                // stretch Y
                // Curve along X
                .addCartesianTransform(BendTransform(
                    constant(boundedScalar(33, 50)),
                    constant(boundedScalar(1, 2))))
                // gentle curve along X
                // Slight barrel
                .addPolarTransform(LensDistortionTransform(
                    constant(boundedScalar(17, 32)))) // slight barrel
                // Slow twist
                .addPolarTransform(VortexTransform(
                    constant(boundedScalar(97, 192))))
                .build();
    }

    PolarPipeline buildTiledMirrorMandalaPreset(const CRGBPalette16 &palette) {
        // Tile, mirror, then fold into a mandala with rotation.
        return PolarPipelineBuilder(noiseLayer, palette, "TiledMirrorMandala")
                // Tile domain
                .addCartesianTransform(TilingTransform(1u << 18, 1u << 18)) // tile at moderate scale
                // Slow drift to keep features moving
                .addCartesianTransform(TranslationTransform(
                    BoundedScalarModulator(BoundedScalar(0),
                                           BoundedScalar(0),
                                           constant(bound(unboundedScalar(1, 2),
                                                          BoundedScalarModulator::SPEED_MIN,
                                                          BoundedScalarModulator::SPEED_MAX)),
                                           constant(BoundedAngle(900)))))
                // Mirror X for symmetry
                .addCartesianTransform(MirrorTransform(true, false))
                // Slow rotation
                .addPolarTransform(RotationTransform(
                    BoundedAngularModulator(
                        BoundedAngle(0),
                        constant(bound(unboundedScalar(1, 2),
                                       BoundedAngularModulator::SPEED_MIN,
                                       BoundedAngularModulator::SPEED_MAX)))))
                // Mandala folding
                .addPolarTransform(KaleidoscopeTransform(6, true, true))
                .build();
    }

    PolarPipeline buildLiquidMarblePreset(const CRGBPalette16 &palette) {
        // Flowing marble: slow noise warp + shear + anisotropic stretch + gentle vortex.
        return PolarPipelineBuilder(noiseLayer, palette, "LiquidMarble")
                // Low-amplitude warp
                .addCartesianTransform(NoiseWarpTransform(
                    constant(boundedScalar(7, 12)),
                    constant(boundedScalar(9, 16))))
                // Skew the domain
                .addCartesianTransform(ShearTransform(
                    constant(boundedScalar(2, 3)),
                    constant(boundedScalar(3, 8))))
                // Compress Y
                .addCartesianTransform(AnisotropicScaleTransform(
                    constant(boundedScalar(1, 4)),
                    constant(boundedScalar(3, 20))))
                // stretch Y ~0.6
                // Gentle twist
                .addPolarTransform(VortexTransform(
                    constant(boundedScalar(65, 128))))
                // Symmetry
                .addPolarTransform(KaleidoscopeTransform(4, false, true))
                .build();
    }

    PolarPipeline buildHeatShimmerPreset(const CRGBPalette16 &palette) {
        // Subtle bend + lens distortion + slow rotation for heat shimmer.
        return PolarPipelineBuilder(fBmLayer, palette, "HeatShimmer")
                // Slight curvature
                .addCartesianTransform(BendTransform(
                    constant(boundedScalar(5, 8)),
                    constant(boundedScalar(1, 2))))
                // Light barrel
                .addPolarTransform(LensDistortionTransform(
                    constant(boundedScalar(13, 24)))) // slight barrel
                // Slow spin
                .addPolarTransform(RotationTransform(
                    BoundedAngularModulator(
                        BoundedAngle(0),
                        constant(bound(unboundedScalar(1, 16),
                                       BoundedAngularModulator::SPEED_MIN,
                                       BoundedAngularModulator::SPEED_MAX)))))
                .build();
    }

    PolarPipeline buildCRTRipplePreset(const CRGBPalette16 &palette) {
        // Tiled noise with noise warp and oscillating radial scale for CRT-like screen wobble.
        return PolarPipelineBuilder(noiseLayer, palette, "CRTRipple")
                // Tile domain
                .addCartesianTransform(TilingTransform(1u << 17, 1u << 17))
                // Subtle warp
                .addCartesianTransform(NoiseWarpTransform(
                    constant(boundedScalar(9, 16)),
                    constant(boundedScalar(13, 24))))
                // Breathing zoom
                .addPolarTransform(RadialScaleTransform(
                    sine(
                        constant(boundedScalar(1, 6)),
                        constant(BoundedScalar(500)))))
                // oscillating zoom, raw 500 is tiny fraction
                // Mild symmetry
                .addPolarTransform(KaleidoscopeTransform(3, false, false))
                .build();
    }

    PolarPipeline buildSpiralGalaxyPreset(const CRGBPalette16 &palette) {
        // Slanted arms with vortex and mandala symmetry.
        return PolarPipelineBuilder(fBmLayer, palette, "SpiralGalaxy")
                // Slant arms
                .addCartesianTransform(ShearTransform(
                    constant(boundedScalar(3, 4)),
                    constant(boundedScalar(1, 2))))
                // Strong twist
                .addPolarTransform(VortexTransform(
                    constant(boundedScalar(25, 48))))
                // Pincushion
                .addPolarTransform(LensDistortionTransform(
                    constant(boundedScalar(9, 20)))) // mild pincushion
                // Slow spin
                .addPolarTransform(RotationTransform(
                    BoundedAngularModulator(
                        BoundedAngle(0),
                        constant(bound(unboundedScalar(1, 8),
                                       BoundedAngularModulator::SPEED_MIN,
                                       BoundedAngularModulator::SPEED_MAX)))))
                // Mandala fold
                .addPolarTransform(KaleidoscopeTransform(4, true, true))
                .build();
    }

    PolarPipeline buildElectricTunnelPreset(const CRGBPalette16 &palette) {
        // Strong stretch, warp, radial push, fast rotation for a tunnel vibe.
        return PolarPipelineBuilder(fBmLayer, palette, "ElectricTunnel")
                // Heavy Y stretch
                .addCartesianTransform(AnisotropicScaleTransform(
                    constant(boundedScalar(1, 4)),
                    constant(boundedScalar(3, 4))))
                // heavy Y stretch
                // Intense warp
                .addCartesianTransform(NoiseWarpTransform(
                    constant(boundedScalar(5, 8)),
                    constant(boundedScalar(3, 5))))
                // Outward push
                .addPolarTransform(RadialScaleTransform(
                    constant(boundedAngle(7, 12))))
                // Fast rotation
                .addPolarTransform(RotationTransform(
                    BoundedAngularModulator(
                        BoundedAngle(0),
                        constant(bound(unboundedScalar(3, 2),
                                       BoundedAngularModulator::SPEED_MIN,
                                       BoundedAngularModulator::SPEED_MAX)))))
                // Mandala symmetry
                .addPolarTransform(KaleidoscopeTransform(5, true, true))
                .build();
    }

    PolarPipeline buildStarburstPulsePreset(const CRGBPalette16 &palette) {
        // Pulsing radial scale with kaleidoscope and barrel distortion.
        return PolarPipelineBuilder(noiseLayer, palette, "StarburstPulse")
                // Pulsed radial zoom
                .addPolarTransform(RadialScaleTransform(
                    pulse(
                        constant(boundedScalar(1, 5)),
                        constant(BoundedScalar(2000)))))
                // Angular symmetry
                .addPolarTransform(KaleidoscopeTransform(6, false, true))
                // Barrel pop
                .addPolarTransform(LensDistortionTransform(
                    constant(boundedScalar(5, 9))))
                .build();
    }

    PolarPipeline buildRainShearPreset(const CRGBPalette16 &palette) {
        // Strong shear/stretch to create slanted rain bands with subtle warp.
        return PolarPipelineBuilder(noiseLayer, palette, "RainShear")
                // Heavy shear
                .addCartesianTransform(ShearTransform(
                    constant(boundedScalar(5, 6)),
                    constant(boundedScalar(1, 2))))
                // heavy X shear
                // Tall stretch
                .addCartesianTransform(AnisotropicScaleTransform(
                    constant(boundedScalar(1, 4)),
                    constant(boundedScalar(1, 1))))
                // tall Y
                // Light warp
                .addCartesianTransform(NoiseWarpTransform(
                    constant(boundedScalar(11, 20)),
                    constant(boundedScalar(17, 32))))
                // Gentle lens
                .addPolarTransform(LensDistortionTransform(
                    constant(boundedScalar(13, 24))))
                .build();
    }

    PolarPipeline buildRippleRingPreset(const CRGBPalette16 &palette) {
        // Sinusoidal radial scale with symmetry and slow rotation.
        return PolarPipelineBuilder(fBmLayer, palette, "RippleRing")
                // Oscillating radial scale
                .addPolarTransform(RadialScaleTransform(
                    sine(
                        constant(boundedScalar(1, 8)),
                        constant(BoundedScalar(1500)))))
                // Mirror both axes (Cartesian symmetry before polar conversion)
                .addCartesianTransform(MirrorTransform(true, true))
                // Many petals
                .addPolarTransform(KaleidoscopeTransform(8, false, false))
                // Slow spin
                .addPolarTransform(RotationTransform(
                    BoundedAngularModulator(
                        BoundedAngle(0),
                        constant(bound(unboundedScalar(1, 12),
                                       BoundedAngularModulator::SPEED_MIN,
                                       BoundedAngularModulator::SPEED_MAX)))))
                .build();
    }

    PolarPipeline buildFractalTileBloomPreset(const CRGBPalette16 &palette) {
        // Tiled fBm with mirror and mandala folding for blooming symmetry.
        return PolarPipelineBuilder(fBmLayer, palette, "FractalTileBloom")
                // Tile domain
                .addCartesianTransform(TilingTransform(1u << 17, 1u << 17))
                // Break up with warp
                .addCartesianTransform(NoiseWarpTransform(
                    constant(boundedScalar(9, 16)),
                    constant(boundedScalar(9, 16))))
                // Mirror X
                .addCartesianTransform(MirrorTransform(true, false))
                // Small bend
                .addCartesianTransform(BendTransform(
                    constant(boundedScalar(2, 3)),
                    constant(boundedScalar(1, 2))))
                // Mandala
                .addPolarTransform(KaleidoscopeTransform(6, true, true))
                // Pincushion
                .addPolarTransform(LensDistortionTransform(
                    constant(boundedScalar(11, 24))))
                .build();
    }

    PolarPipeline buildStutterPulsePreset(const CRGBPalette16 &palette) {
        // Pulsing radial scale with medium rotation and kaleidoscope; stutter comes from pulsed radius.
        return PolarPipelineBuilder(noiseLayer, palette, "StutterPulse")
                // Pulsed radius
                .addPolarTransform(RadialScaleTransform(
                    pulse(
                        constant(boundedScalar(1, 4)),
                        constant(BoundedScalar(2500)))))
                // Medium rotation
                .addPolarTransform(RotationTransform(
                    BoundedAngularModulator(
                        BoundedAngle(0),
                        constant(bound(unboundedScalar(1, 2),
                                       BoundedAngularModulator::SPEED_MIN,
                                       BoundedAngularModulator::SPEED_MAX)))))
                // Symmetry
                .addPolarTransform(KaleidoscopeTransform(5, false, true))
                // Lens
                .addPolarTransform(LensDistortionTransform(
                    constant(boundedScalar(11, 20))))
                .build();
    }

    PolarPipeline buildCurlFlowSmokePreset(const CRGBPalette16 &palette) {
        // Divergence-free curl flow for fluid smoke-like advection with light vortex and lens.
        return PolarPipelineBuilder(fBmLayer, palette, "CurlFlowSmoke")
                // Curl advection
                .addCartesianTransform(CurlFlowTransform(
                    constant(boundedScalar(7, 12)),
                    3))
                // Additional warp
                .addCartesianTransform(NoiseWarpTransform(
                    constant(boundedScalar(11, 20)),
                    constant(boundedScalar(11, 20))))
                // Twist
                .addPolarTransform(VortexTransform(
                    constant(boundedScalar(81, 160))))
                // Lens
                .addPolarTransform(LensDistortionTransform(
                    constant(boundedScalar(13, 24))))
                .build();
    }

    PolarPipeline buildPerspectiveDepthPreset(const CRGBPalette16 &palette) {
        // Perspective warp to fake depth, then kaleidoscope symmetry.
        return PolarPipelineBuilder(noiseLayer, palette, "PerspectiveDepth")
                // Perspective scale
                .addCartesianTransform(PerspectiveWarpTransform(
                    constant(boundedScalar(2, 3))))
                // Slight shear
                .addCartesianTransform(ShearTransform(
                    constant(boundedScalar(9, 14)),
                    constant(boundedScalar(1, 2))))
                // Symmetry
                .addPolarTransform(KaleidoscopeTransform(4, false, true))
                // Slow spin
                .addPolarTransform(RotationTransform(
                    BoundedAngularModulator(
                        BoundedAngle(0),
                        constant(bound(unboundedScalar(1, 10),
                                       BoundedAngularModulator::SPEED_MIN,
                                       BoundedAngularModulator::SPEED_MAX)))))
                .build();
    }

    PolarPipeline buildPosterizedRingsPreset(const CRGBPalette16 &palette) {
        // Quantized rings/sectors feeding into twist and rotation.
        return PolarPipelineBuilder(fBmLayer, palette, "PosterizedRings")
                // Quantize angle/radius
                .addPolarTransform(PosterizePolarTransform(12, 6))
                // Twist
                .addPolarTransform(VortexTransform(
                    constant(boundedScalar(41, 80))))
                // Rotation
                .addPolarTransform(RotationTransform(
                    BoundedAngularModulator(
                        BoundedAngle(0),
                        constant(bound(unboundedScalar(1, 6),
                                       BoundedAngularModulator::SPEED_MIN,
                                       BoundedAngularModulator::SPEED_MAX)))))
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
                    constant(boundedScalar(1, 1)) // constant amplitude via signal
                ))
                // Mandala fold
                .addPolarTransform(KaleidoscopeTransform(5, true, true))
                .build();
    }
}
