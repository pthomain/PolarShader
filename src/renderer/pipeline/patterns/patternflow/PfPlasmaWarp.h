//  SPDX-License-Identifier: GPL-3.0-only

/*
 * PatternFlow adaptation NOTICE
 * -----------------------------
 * The field algorithms in this file are adapted from the PatternFlow project.
 *   Upstream:  engmung/Patternflow
 *   Author:    Seunghun LEE
 *   Licence:   CC-BY-SA-4.0 (upstream pattern licence)
 *   Effects:   "A Big Hit" (Plasma), 0520 (Tendrils), 0530 (LiquidGate)
 * Only the algorithm / field idea is adapted (re-derived into PolarShader's
 * fixed-point, palette-driven, UV-space pipeline); no source lines are copied.
 * The upstream HSV colour is dropped: this pattern emits scalar intensity.
 * See patternflow/ATTRIBUTION.md. Distributed under GPL-3.0-only.
 */

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFPLASMAWARP_H
#define POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFPLASMAWARP_H

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include <memory>

namespace PolarShader {
    /**
     * Domain-warped plasma field: sine waves whose input coordinates are
     * displaced by other sine waves (self-warping), producing organic blobs
     * and filaments.
     *
     *   - Plasma    ("A Big Hit"): four warped waves -> soft bright plasma.
     *   - Tendrils  (0520):        self-warped centre-field -> thin filaments.
     *   - LiquidGate (0530): a warped sine*cos product field pushed through a
     *     drifting contrast gate -> high-contrast liquid metaball blobs.
     *
     * Intrinsic parameters only. Global scale/density -> ZoomTransform,
     * colour -> PaletteTransform live in the preset.
     *
     * Parameter ranges (signals sampled in magnitudeRange() -> [0, 1]):
     *   phaseSpeed [0,1] -> internal animation rate.
     *   warp       [0,1] -> self-warp / chaos depth of the domain distortion.
     *   thickness  [0,1] -> filament width (Tendrils); mild plasma softness;
     *              LiquidGate: gate transition width (contrast softness).
     */
    class PfPlasmaWarp : public UVPattern {
    public:
        enum class Variant : uint8_t {
            Plasma,
            Tendrils,
            LiquidGate
        };

        explicit PfPlasmaWarp(
            Variant variant = Variant::Plasma,
            Sf16Signal phaseSpeed = constant(500),
            Sf16Signal warp = constant(500),
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

#endif // POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFPLASMAWARP_H
