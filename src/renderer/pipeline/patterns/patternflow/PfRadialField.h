//  SPDX-License-Identifier: GPL-3.0-only

/*
 * PatternFlow adaptation NOTICE
 * -----------------------------
 * The field algorithm in this file is adapted from the PatternFlow project.
 *   Upstream:  engmung/Patternflow
 *   Author:    Seunghun LEE
 *   Licence:   CC-BY-SA-4.0 (upstream pattern licence)
 *   Effects:   0512 (Petals), 0524-2 (Ripple)
 * Only the algorithm / field idea is adapted (re-derived into PolarShader's
 * fixed-point, palette-driven, UV-space pipeline); no source lines are copied.
 * The upstream HSV colour is dropped: this pattern emits scalar intensity.
 * See patternflow/ATTRIBUTION.md. Distributed under GPL-3.0-only.
 */

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFRADIALFIELD_H
#define POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFRADIALFIELD_H

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include <memory>

namespace PolarShader {
    /**
     * Radially-symmetric petal / ring field.
     *
     *   - Petals (0512): a pulsing flower whose radial target distance is
     *     modulated by an angular petal wave plus a radial ripple.
     *   - Ripple (0524-2): concentric refraction-like ripples whose wavefronts
     *     are angularly bent by a warp term, read out as thin bright crests.
     *   - Chirp: a sonar sweep whose radial frequency rises with radius
     *     (rings densify toward the edge); fold sets the chirp rate.
     *   - SpiralArms: a radius-sheared Archimedean pinwheel of galaxy arms
     *     (armCount arms; fold = winding tightness). No true log spiral (no
     *     ln() in fixed point) -> arms use a radial shear instead.
     *
     * Intrinsic parameters only. Global rotation -> RotationTransform,
     * scale -> ZoomTransform, colour -> PaletteTransform live in the preset.
     * The Ripple variant is the base for pfKaleidoVortexPreset (0526), which
     * adds RadialKaleidoscope + Vortex transforms downstream.
     *
     * Parameter ranges (signals sampled in magnitudeRange() -> [0, 1]):
     *   phaseSpeed [0,1] -> internal animation rate.
     *   fold       [0,1] -> Petals: depth of the radial ripple on the petals;
     *              Ripple: angular warp depth that bends the wavefronts;
     *              Chirp: extra radial frequency gain toward the edge;
     *              SpiralArms: arm winding tightness.
     *   thickness  [0,1] -> width of the lit petal band / ripple crest / ring /
     *              spiral arm.
     * petalCount is structural: petals around the circle / radial wave count /
     * chirp ring count / spiral arm count.
     */
    class PfRadialField : public UVPattern {
    public:
        enum class Variant : uint8_t {
            Petals,
            Ripple,
            Chirp,
            SpiralArms
        };

        explicit PfRadialField(
            Variant variant = Variant::Petals,
            uint8_t petalCount = 6,
            Sf16Signal phaseSpeed = constant(500),
            Sf16Signal fold = constant(500),
            Sf16Signal thickness = constant(500)
        );

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;

        UVMap layer(const std::shared_ptr<PipelineContext> &context) const override;

        bool emitsColour() const override;

        UVColourMap colourLayer(const std::shared_ptr<PipelineContext> &context) const override;

    private:
        struct State;
        struct Functor;
        std::shared_ptr<State> state;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFRADIALFIELD_H
