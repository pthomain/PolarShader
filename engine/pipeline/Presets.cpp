//  SPDX-License-Identifier: GPL-3.0-or-later
//  Copyright (C) 2024 Pierre Thomain

#include "Presets.h"
#include "CartesianNoiseLayers.h"
#include "transforms/DomainWarpTransform.h"
#include "transforms/RotationTransform.h"
#include "transforms/VortexTransform.h"
#include "transforms/KaleidoscopeTransform.h"
#include "transforms/LensDistortionTransform.h"
#include "transforms/ShearTransform.h"
#include "transforms/AnisotropicScaleTransform.h"
#include "transforms/BendTransform.h"
#include "transforms/RadialScaleTransform.h"
#include "transforms/TilingTransform.h"
#include "transforms/MirrorTransform.h"
#include "transforms/NoiseWarpTransform.h"
#include "transforms/CurlFlowTransform.h"
#include "transforms/PerspectiveWarpTransform.h"
#include "transforms/PosterizePolarTransform.h"
#include "transforms/TimeQuantizeTransform.h"
#include "transforms/TileJitterTransform.h"
#include "signals/Waveforms.h"

namespace LEDSegments {
    namespace {
        // Helper to create Q16.16 values from rational fractions without floating point.
        constexpr Units::SignalQ16_16 q16_frac(int32_t num, uint32_t den) {
            return Units::SignalQ16_16::fromRaw(static_cast<int32_t>((static_cast<int64_t>(num) << 16) / static_cast<int64_t>(den)));
        }
    }

    PolarPipeline buildBarrelTunnelPreset(const CRGBPalette16 &palette) {
        // Mild domain warp + barrel distortion + kaleidoscope symmetry.
        return PolarPipelineBuilder(fBmLayer, palette, "BarrelTunnel")
                // Low-frequency domain drift
                .addCartesianTransform(
                    DomainWarpTransform(
                        LinearSignal(0, {Waveforms::ConstantAccelerationWaveform(0), Waveforms::ConstantPhaseVelocityWaveform(1500)}),
                        LinearSignal(0, {Waveforms::ConstantAccelerationWaveform(0), Waveforms::ConstantPhaseVelocityWaveform(-1200)})
                    )
                )
                // Barrel distortion
                .addPolarTransform(LensDistortionTransform(LinearSignal(q16_frac(1, 64)))) // subtle barrel
                // Gentle outward zoom
                .addPolarTransform(RadialScaleTransform(LinearSignal(q16_frac(1, 32)))) // subtle outward zoom
                // Angular symmetry
                .addPolarTransform(KaleidoscopeTransform(5, false, true))
                .build();
    }

    PolarPipeline buildNoiseWarpFlamePreset(const CRGBPalette16 &palette) {
        // Noise-driven warp, anisotropic stretch, gentle bend, and barrel distortion.
        return PolarPipelineBuilder(fBmLayer, palette, "NoiseWarpFlame")
                // Turbulent warp
                .addCartesianTransform(NoiseWarpTransform(LinearSignal(q16_frac(1, 20)), LinearSignal(q16_frac(3, 100))))
                // Stretch vertically
                .addCartesianTransform(AnisotropicScaleTransform(LinearSignal(1 << 16), LinearSignal(2 << 16)))
                // stretch Y
                // Curve along X
                .addCartesianTransform(BendTransform(LinearSignal(q16_frac(1, 100)), LinearSignal(0))) // gentle curve along X
                // Slight barrel
                .addPolarTransform(LensDistortionTransform(LinearSignal(q16_frac(1, 128)))) // slight barrel
                // Slow twist
                .addPolarTransform(VortexTransform(BoundedSignal(
                    Units::SignalQ16_16(0), Waveforms::Constant(0), 0,
                    q16_frac(-1, 64), q16_frac(1, 64))))
                .build();
    }

    PolarPipeline buildTiledMirrorMandalaPreset(const CRGBPalette16 &palette) {
        // Tile, mirror, then fold into a mandala with rotation.
        return PolarPipelineBuilder(noiseLayer, palette, "TiledMirrorMandala")
                // Tile domain
                .addCartesianTransform(TilingTransform(1u << 18, 1u << 18)) // tile at moderate scale
                // Mirror X for symmetry
                .addCartesianTransform(MirrorTransform(true, false))
                // Slow rotation
                .addPolarTransform(RotationTransform(AngularSignal(Units::SignalQ16_16(0),
                                                                   {Waveforms::ConstantAccelerationWaveform(0), Waveforms::ConstantPhaseVelocityWaveform(1800)})))
                // Mandala folding
                .addPolarTransform(KaleidoscopeTransform(6, true, true))
                .build();
    }

    PolarPipeline buildLiquidMarblePreset(const CRGBPalette16 &palette) {
        // Flowing marble: slow noise warp + shear + anisotropic stretch + gentle vortex.
        return PolarPipelineBuilder(noiseLayer, palette, "LiquidMarble")
                // Low-amplitude warp
                .addCartesianTransform(NoiseWarpTransform(LinearSignal(q16_frac(1, 24)), LinearSignal(q16_frac(1, 32))))
                // Skew the domain
                .addCartesianTransform(ShearTransform(LinearSignal(q16_frac(1, 12)), LinearSignal(q16_frac(-1, 16))))
                // Compress Y
                .addCartesianTransform(AnisotropicScaleTransform(LinearSignal(1 << 16), LinearSignal(q16_frac(3, 5))))
                // stretch Y ~0.6
                // Gentle twist
                .addPolarTransform(VortexTransform(BoundedSignal(
                    Units::SignalQ16_16(0), Waveforms::Constant(0), 0,
                    q16_frac(-1, 48), q16_frac(1, 48))))
                // Symmetry
                .addPolarTransform(KaleidoscopeTransform(4, false, true))
                .build();
    }

    PolarPipeline buildHeatShimmerPreset(const CRGBPalette16 &palette) {
        // Subtle bend + lens distortion + slow rotation for heat shimmer.
        return PolarPipelineBuilder(fBmLayer, palette, "HeatShimmer")
                // Slight curvature
                .addCartesianTransform(BendTransform(LinearSignal(q16_frac(1, 128)), LinearSignal(0)))
                // Light barrel
                .addPolarTransform(LensDistortionTransform(LinearSignal(q16_frac(1, 96)))) // slight barrel
                // Slow spin
                .addPolarTransform(RotationTransform(AngularSignal(Units::SignalQ16_16(0),
                                                                   {Waveforms::ConstantAccelerationWaveform(0), Waveforms::ConstantPhaseVelocityWaveform(900)})))
                .build();
    }

    PolarPipeline buildCRTRipplePreset(const CRGBPalette16 &palette) {
        // Tiled noise with noise warp and oscillating radial scale for CRT-like screen wobble.
        return PolarPipelineBuilder(noiseLayer, palette, "CRTRipple")
                // Tile domain
                .addCartesianTransform(TilingTransform(1u << 17, 1u << 17))
                // Subtle warp
                .addCartesianTransform(NoiseWarpTransform(LinearSignal(q16_frac(1, 32)), LinearSignal(q16_frac(1, 48))))
                // Breathing zoom
                .addPolarTransform(RadialScaleTransform(
                    LinearSignal(Units::SignalQ16_16(0),
                                 Waveforms::Sine(Waveforms::ConstantPhaseVelocityWaveform(30),
                                                 Waveforms::ConstantAccelerationWaveformRaw(500))))) // oscillating zoom, raw 500 is tiny fraction
                // Mild symmetry
                .addPolarTransform(KaleidoscopeTransform(3, false, false))
                .build();
    }

    PolarPipeline buildSpiralGalaxyPreset(const CRGBPalette16 &palette) {
        // Slanted arms with vortex and mandala symmetry.
        return PolarPipelineBuilder(fBmLayer, palette, "SpiralGalaxy")
                // Slant arms
                .addCartesianTransform(ShearTransform(LinearSignal(q16_frac(1, 8)), LinearSignal(0)))
                // Strong twist
                .addPolarTransform(VortexTransform(BoundedSignal(
                    Units::SignalQ16_16(0), Waveforms::Constant(0), 0,
                    q16_frac(1, 80), q16_frac(3, 40))))
                // Pincushion
                .addPolarTransform(LensDistortionTransform(LinearSignal(q16_frac(-1, 80)))) // mild pincushion
                // Slow spin
                .addPolarTransform(RotationTransform(AngularSignal(Units::SignalQ16_16(0),
                                                                   {Waveforms::ConstantAccelerationWaveform(0), Waveforms::ConstantPhaseVelocityWaveform(1600)})))
                // Mandala fold
                .addPolarTransform(KaleidoscopeTransform(4, true, true))
                .build();
    }

    PolarPipeline buildElectricTunnelPreset(const CRGBPalette16 &palette) {
        // Strong stretch, warp, radial push, fast rotation for a tunnel vibe.
        return PolarPipelineBuilder(fBmLayer, palette, "ElectricTunnel")
                // Heavy Y stretch
                .addCartesianTransform(AnisotropicScaleTransform(LinearSignal(1 << 16), LinearSignal(3 << 16)))
                // heavy Y stretch
                // Intense warp
                .addCartesianTransform(NoiseWarpTransform(LinearSignal(q16_frac(1, 16)), LinearSignal(q16_frac(1, 20))))
                // Outward push
                .addPolarTransform(RadialScaleTransform(LinearSignal(q16_frac(1, 12))))
                // Fast rotation
                .addPolarTransform(RotationTransform(AngularSignal(Units::SignalQ16_16(0),
                                                                   {Waveforms::ConstantAccelerationWaveform(0), Waveforms::ConstantPhaseVelocityWaveform(4500)})))
                // Mandala symmetry
                .addPolarTransform(KaleidoscopeTransform(5, true, true))
                .build();
    }

    PolarPipeline buildStarburstPulsePreset(const CRGBPalette16 &palette) {
        // Pulsing radial scale with kaleidoscope and barrel distortion.
        return PolarPipelineBuilder(noiseLayer, palette, "StarburstPulse")
                // Pulsed radial zoom
                .addPolarTransform(RadialScaleTransform(
                    LinearSignal(Units::SignalQ16_16(0), Waveforms::Pulse(Waveforms::ConstantPhaseVelocityWaveform(40),
                                                                         Waveforms::ConstantAccelerationWaveformRaw(2000)))))
                // Angular symmetry
                .addPolarTransform(KaleidoscopeTransform(6, false, true))
                // Barrel pop
                .addPolarTransform(LensDistortionTransform(LinearSignal(q16_frac(1, 72))))
                .build();
    }

    PolarPipeline buildRainShearPreset(const CRGBPalette16 &palette) {
        // Strong shear/stretch to create slanted rain bands with subtle warp.
        return PolarPipelineBuilder(noiseLayer, palette, "RainShear")
                // Heavy shear
                .addCartesianTransform(ShearTransform(LinearSignal(q16_frac(1, 6)), LinearSignal(0))) // heavy X shear
                // Tall stretch
                .addCartesianTransform(AnisotropicScaleTransform(LinearSignal(1 << 16), LinearSignal(4 << 16)))
                // tall Y
                // Light warp
                .addCartesianTransform(NoiseWarpTransform(LinearSignal(q16_frac(1, 40)), LinearSignal(q16_frac(1, 64))))
                // Gentle lens
                .addPolarTransform(LensDistortionTransform(LinearSignal(q16_frac(1, 96))))
                .build();
    }

    PolarPipeline buildRippleRingPreset(const CRGBPalette16 &palette) {
        // Sinusoidal radial scale with symmetry and slow rotation.
        return PolarPipelineBuilder(fBmLayer, palette, "RippleRing")
                // Oscillating radial scale
                .addPolarTransform(RadialScaleTransform(LinearSignal(
                    Units::SignalQ16_16(0), Waveforms::Sine(Waveforms::ConstantPhaseVelocityWaveform(25), Waveforms::ConstantAccelerationWaveformRaw(1500)))))
                // Mirror both axes (Cartesian symmetry before polar conversion)
                .addCartesianTransform(MirrorTransform(true, true))
                // Many petals
                .addPolarTransform(KaleidoscopeTransform(8, false, false))
                // Slow spin
                .addPolarTransform(RotationTransform(AngularSignal(Units::SignalQ16_16(0),
                                                                   {Waveforms::ConstantAccelerationWaveform(0), Waveforms::ConstantPhaseVelocityWaveform(1200)})))
                .build();
    }

    PolarPipeline buildFractalTileBloomPreset(const CRGBPalette16 &palette) {
        // Tiled fBm with mirror and mandala folding for blooming symmetry.
        return PolarPipelineBuilder(fBmLayer, palette, "FractalTileBloom")
                // Tile domain
                .addCartesianTransform(TilingTransform(1u << 17, 1u << 17))
                // Break up with warp
                .addCartesianTransform(NoiseWarpTransform(LinearSignal(q16_frac(1, 32)), LinearSignal(q16_frac(1, 32))))
                // Mirror X
                .addCartesianTransform(MirrorTransform(true, false))
                // Small bend
                .addCartesianTransform(BendTransform(LinearSignal(q16_frac(1, 96)), LinearSignal(0)))
                // Mandala
                .addPolarTransform(KaleidoscopeTransform(6, true, true))
                // Pincushion
                .addPolarTransform(LensDistortionTransform(LinearSignal(q16_frac(-1, 96))))
                .build();
    }

    PolarPipeline buildStutterPulsePreset(const CRGBPalette16 &palette) {
        // Pulsing radial scale with medium rotation and kaleidoscope; stutter comes from pulsed radius.
        return PolarPipelineBuilder(noiseLayer, palette, "StutterPulse")
                // Pulsed radius
                .addPolarTransform(RadialScaleTransform(LinearSignal(
                    Units::SignalQ16_16(0), Waveforms::Pulse(Waveforms::ConstantPhaseVelocityWaveform(60), Waveforms::ConstantAccelerationWaveformRaw(2500)))))
                // Medium rotation
                .addPolarTransform(RotationTransform(AngularSignal(Units::SignalQ16_16(0),
                                                                   {Waveforms::ConstantAccelerationWaveform(0), Waveforms::ConstantPhaseVelocityWaveform(2500)})))
                // Symmetry
                .addPolarTransform(KaleidoscopeTransform(5, false, true))
                // Lens
                .addPolarTransform(LensDistortionTransform(LinearSignal(q16_frac(1, 80))))
                .build();
    }

    PolarPipeline buildCurlFlowSmokePreset(const CRGBPalette16 &palette) {
        // Divergence-free curl flow for fluid smoke-like advection with light vortex and lens.
        return PolarPipelineBuilder(fBmLayer, palette, "CurlFlowSmoke")
                // Curl advection
                .addCartesianTransform(CurlFlowTransform(LinearSignal(q16_frac(1, 24)), 3))
                // Additional warp
                .addCartesianTransform(NoiseWarpTransform(LinearSignal(q16_frac(1, 40)), LinearSignal(q16_frac(1, 40))))
                // Twist
                .addPolarTransform(VortexTransform(BoundedSignal(
                    Units::SignalQ16_16(0), Waveforms::Constant(0), 0,
                    q16_frac(-1, 64), q16_frac(1, 64))))
                // Lens
                .addPolarTransform(LensDistortionTransform(LinearSignal(q16_frac(1, 96))))
                .build();
    }

    PolarPipeline buildPerspectiveDepthPreset(const CRGBPalette16 &palette) {
        // Perspective warp to fake depth, then kaleidoscope symmetry.
        return PolarPipelineBuilder(noiseLayer, palette, "PerspectiveDepth")
                // Perspective scale
                .addCartesianTransform(PerspectiveWarpTransform(LinearSignal(q16_frac(1, 12))))
                // Slight shear
                .addCartesianTransform(ShearTransform(LinearSignal(q16_frac(1, 14)), LinearSignal(0)))
                // Symmetry
                .addPolarTransform(KaleidoscopeTransform(4, false, true))
                // Slow spin
                .addPolarTransform(RotationTransform(AngularSignal(Units::SignalQ16_16(0),
                                                                   {Waveforms::ConstantAccelerationWaveform(0), Waveforms::ConstantPhaseVelocityWaveform(1200)})))
                .build();
    }

    PolarPipeline buildPosterizedRingsPreset(const CRGBPalette16 &palette) {
        // Quantized rings/sectors feeding into twist and rotation.
        return PolarPipelineBuilder(fBmLayer, palette, "PosterizedRings")
                // Quantize angle/radius
                .addPolarTransform(PosterizePolarTransform(12, 6))
                // Twist
                .addPolarTransform(VortexTransform(BoundedSignal(
                    Units::SignalQ16_16(0), Waveforms::Constant(0), 0,
                    q16_frac(-1, 48), q16_frac(1, 48))))
                // Rotation
                .addPolarTransform(RotationTransform(AngularSignal(Units::SignalQ16_16(0),
                                                                   {Waveforms::ConstantAccelerationWaveform(0), Waveforms::ConstantPhaseVelocityWaveform(1500)})))
                .build();
    }

    PolarPipeline buildJitteredTilesPreset(const CRGBPalette16 &palette) {
        // Tiling with per-tile jitter to break repetition, then mandala.
        return PolarPipelineBuilder(noiseLayer, palette, "JitteredTiles")
                // Tile
                .addCartesianTransform(TilingTransform(1u << 17, 1u << 17))
                // Jitter tiles
                .addCartesianTransform(TileJitterTransform(1u << 17, 1u << 17, 1024))
                // Mandala fold
                .addPolarTransform(KaleidoscopeTransform(5, true, true))
                .build();
    }

    PolarPipeline buildTimeStutterPreset(const CRGBPalette16 &palette) {
        // Apply time quantization for stutter, combined with vortex and lens distortion.
        return PolarPipelineBuilder(fBmLayer, palette, "TimeStutter")
                // Twist
                .addPolarTransform(VortexTransform(BoundedSignal(
                    Units::SignalQ16_16(0), Waveforms::Constant(0), 0,
                    q16_frac(-1, 64), q16_frac(1, 64))))
                // Quantize time
                .addPolarTransform(TimeQuantizeTransform(80))
                // Lens
                .addPolarTransform(LensDistortionTransform(LinearSignal(q16_frac(1, 72))))
                // Symmetry
                .addPolarTransform(KaleidoscopeTransform(4, false, true))
                .build();
    }
}
