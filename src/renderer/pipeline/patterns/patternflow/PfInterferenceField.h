//  SPDX-License-Identifier: GPL-3.0-only

/*
 * PatternFlow adaptation NOTICE
 * -----------------------------
 * The field algorithms in this file are adapted from the PatternFlow project.
 *   Upstream:  engmung/Patternflow
 *   Author:    Seunghun LEE
 *   Licence:   CC-BY-SA-4.0 (upstream pattern licence)
 *   Effects:   0510 (DualAxis), 0515 (CounterRibbons),
 *              0515-3 (QuadDirectional), 0531 (Posterized), 0614-2 (Cross)
 * Only the algorithm / field idea is adapted (re-derived into PolarShader's
 * fixed-point, palette-driven, UV-space pipeline); no source lines are copied.
 * The upstream HSV colour is dropped: this pattern emits scalar intensity.
 * See patternflow/ATTRIBUTION.md for the full licensing rationale.
 *
 * This file is part of PolarShader and is distributed under GPL-3.0-only
 * (the CC-BY-SA-4.0 -> GPLv3 one-way compatibility path). See
 * <https://www.gnu.org/licenses/> and patternflow/ATTRIBUTION.md.
 */

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFINTERFERENCEFIELD_H
#define POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFINTERFERENCEFIELD_H

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include <memory>

namespace PolarShader {
    /**
     * Multi-directional sine interference field.
     *
     * A small family of PatternFlow effects that sum several travelling sine
     * waves oriented along different axes and shape the result. The variant
     * selects the axis set and the output shaping:
     *   - DualAxis        (0510):  two domain-warped orthogonal waves.
     *   - CounterRibbons  (0515):  two counter-travelling threshold ribbons.
     *   - QuadDirectional (0515-3): four axis waves -> sharp bright veins.
     *   - Posterized      (0531):  three waves quantised into flat bands.
     *   - Cross           (0614-2): a breathing plus/cross of axis-aligned
     *     bars. The single cross is folded into a grid by a TilingTransform in
     *     pfCrossPreset (the fold is a transform, not a pattern param).
     *
     * Intrinsic parameters only (per the PatternFlow design rule). Global
     * scale/density -> ZoomTransform, colour/hue -> PaletteTransform, tiling/
     * fold -> TilingTransform, and global drift/rotation live in the pf*Preset
     * transform stack, NOT here.
     *
     * Parameter ranges (all signals sampled in magnitudeRange() -> [0, 1]):
     *   phaseSpeed [0,1] -> internal animation rate, up to ~0.4 turns/sec.
     *   warp       [0,1] -> domain-warp / chaos depth of the field (Cross: bar
     *              centre drift).
     *   thickness  [0,1] -> feature (ribbon / vein / cross-bar) width.
     * posterizeLevels is structural: the flat-band count for the Posterized
     * variant (>= 2); ignored by the other variants.
     */
    class PfInterferenceField : public UVPattern {
    public:
        enum class Variant : uint8_t {
            DualAxis,
            CounterRibbons,
            QuadDirectional,
            Posterized,
            Cross
        };

        explicit PfInterferenceField(
            Variant variant = Variant::DualAxis,
            uint8_t posterizeLevels = 5,
            Sf16Signal phaseSpeed = constant(500),
            Sf16Signal warp = constant(500),
            Sf16Signal thickness = constant(500)
        );

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;

        UVMap layer(const std::shared_ptr<PipelineContext> &context) const override;

    private:
        struct State;
        struct Functor;
        std::shared_ptr<State> state;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFINTERFERENCEFIELD_H
