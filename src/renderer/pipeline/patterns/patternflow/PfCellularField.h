//  SPDX-License-Identifier: GPL-3.0-only

/*
 * PatternFlow adaptation NOTICE
 * -----------------------------
 * The field algorithms in this file are adapted from the PatternFlow project.
 *   Upstream:  engmung/Patternflow
 *   Author:    Seunghun LEE
 *   Licence:   CC-BY-SA-4.0 (upstream pattern licence)
 *   Effects:   Origin (ConcentricGrid), 0511 (RowSegments, approx),
 *              0514 (Shapes), 0528 (Dots), 0601 (WaveMatrix),
 *              0624 (RadialPulse, approx)
 * Only the algorithm / field idea is adapted (re-derived into PolarShader's
 * fixed-point, palette-driven, UV-space pipeline); no source lines are copied.
 * The upstream HSV colour is dropped: this pattern emits scalar intensity.
 * 0511 (RowSegments) and 0624 (RadialPulse) are idiomatic approximations, not
 * faithful ports (0624's nested isometric pillars are re-expressed as a radial
 * cell-height pulse field).
 * See patternflow/ATTRIBUTION.md. Distributed under GPL-3.0-only.
 */

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFCELLULARFIELD_H
#define POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFCELLULARFIELD_H

#include "renderer/pipeline/patterns/base/UVPattern.h"
#include "renderer/pipeline/signals/Signals.h"
#include "renderer/pipeline/signals/SignalTypes.h"
#include <memory>

namespace PolarShader {
    /**
     * Grid-cell field family: the plane is partitioned into cellCount x
     * cellCount cells and each cell renders a local motif.
     *
     *   - ConcentricGrid (Origin): per-cell radial rings.
     *   - RowSegments (0511, approx): hashed sliding row segments.
     *   - Shapes (0514): warped per-cell blobs.
     *   - Dots (0528): energy-gated circular dots.
     *   - WaveMatrix (0601): per-cell phase-shifted wave gate.
     *   - RadialPulse (0624, approx): per-cell pillars whose height pulses
     *     outward from the grid centre (nested isometric pillars re-expressed
     *     as a radial cell-height field).
     *
     * Intrinsic parameters only. Global cell scale is the structural cellCount
     * (a genuine topology control, not a zoom duplicate); colour -> Palette
     * transform lives in the preset.
     *
     * Parameter ranges (signals sampled in magnitudeRange() -> [0, 1]):
     *   phaseSpeed [0,1] -> internal animation rate.
     *   warp       [0,1] -> per-cell chaos / energy jitter / phase split.
     *   thickness  [0,1] -> motif size / activation threshold (per variant).
     * cellCount is structural: cells per axis (>= 1).
     */
    class PfCellularField : public UVPattern {
    public:
        enum class Variant : uint8_t {
            ConcentricGrid,
            RowSegments,
            Shapes,
            Dots,
            WaveMatrix,
            RadialPulse
        };

        explicit PfCellularField(
            Variant variant = Variant::ConcentricGrid,
            uint8_t cellCount = 6,
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

#endif // POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PFCELLULARFIELD_H
