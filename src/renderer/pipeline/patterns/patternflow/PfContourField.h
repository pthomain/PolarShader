//  SPDX-License-Identifier: GPL-3.0-only

/*
 * PatternFlow adaptation NOTICE
 * -----------------------------
 * The field algorithm in this file is adapted from the PatternFlow project.
 *   Upstream:  engmung/Patternflow
 *   Author:    Seunghun LEE
 *   Licence:   CC-BY-SA-4.0 (upstream pattern licence)
 *   Effects:   0519-1 (Organic), 0529 (Topographic)
 * Only the algorithm / field idea is adapted (re-derived into PolarShader's
 * fixed-point, palette-driven, UV-space pipeline); no source lines are copied.
 * The upstream HSV colour is dropped: this pattern emits scalar intensity.
 * See patternflow/ATTRIBUTION.md. Distributed under GPL-3.0-only.
 */

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFCONTOURFIELD_H
#define POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFCONTOURFIELD_H

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include <memory>

namespace PolarShader {
    /**
     * Iso-contour ("topographic") field: a smooth scalar field sliced into
     * evenly-spaced contour lines.
     *
     *   - Organic (0519-1): four axis waves summed into a slowly drifting
     *     height field, then banded into contour ridges.
     *   - Topographic (0529): the same height field read out as thin
     *     iso-contour lines (level-set proximity), like a topographic map.
     *
     * Intrinsic parameters only. Global scale -> ZoomTransform,
     * colour -> PaletteTransform live in the preset.
     *
     * Parameter ranges (signals sampled in magnitudeRange() -> [0, 1]):
     *   phaseSpeed [0,1] -> internal animation rate.
     *   tension    [0,1] -> Organic: DC bias of the height field (shifts which
     *              elevations the contour lines sit on); Topographic: softness /
     *              width of the iso-contour lines.
     * contourLevels is structural: the number of contour bands (>= 1).
     * hardEdges toggles crisp on/off contour lines vs. soft ridges.
     */
    class PfContourField : public UVPattern {
    public:
        enum class Variant : uint8_t {
            Organic,
            Topographic
        };

        explicit PfContourField(
            Variant variant = Variant::Organic,
            uint8_t contourLevels = 10,
            bool hardEdges = false,
            Sf16Signal phaseSpeed = constant(500),
            Sf16Signal tension = constant(500)
        );

        void advanceFrame(f16 progress, TimeMillis elapsedMs) override;

        UVMap layer(const std::shared_ptr<PipelineContext> &context) const override;

    private:
        struct State;
        struct Functor;
        std::shared_ptr<State> state;
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFCONTOURFIELD_H
