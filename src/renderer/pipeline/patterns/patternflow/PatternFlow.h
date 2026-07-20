//  SPDX-License-Identifier: GPL-3.0-only

/*
 * PatternFlow adaptation NOTICE
 * -----------------------------
 * Pattern factories for the PatternFlow field family adapted from
 * engmung/Patternflow (author Seunghun LEE, CC-BY-SA-4.0). Only the field
 * algorithms are adapted (re-derived into PolarShader's fixed-point, palette-
 * driven, UV-space pipeline); no source lines are copied. Distributed under
 * GPL-3.0-only via the CC-BY-SA-4.0 -> GPLv3 one-way compatibility path.
 * See patternflow/ATTRIBUTION.md.
 */

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PATTERNFLOW_H
#define POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PATTERNFLOW_H

#include <memory>
#include "renderer/pipeline/patterns/patternflow/PfInterferenceField.h"
#include "renderer/pipeline/patterns/patternflow/PfRadialField.h"
#include "renderer/pipeline/patterns/patternflow/PfContourField.h"
#include "renderer/pipeline/patterns/patternflow/PfPlasmaWarp.h"
#include "renderer/pipeline/patterns/patternflow/PfCellularField.h"
#include "renderer/pipeline/patterns/patternflow/PfSourceField.h"
#include "renderer/pipeline/signals/Signals.h"

namespace PolarShader {
    /*
     * PatternFlow pattern factories, named by field Variant (the canonical API).
     * Each factory carries a // <date-code> comment tracing it to the upstream
     * effect. Intrinsic parameters only; global coordinate/palette controls live
     * in the pf*Preset transform stacks (see PatternFlowPresets.h).
     */

    // --- PfInterferenceField ---
    std::unique_ptr<UVPattern> pfDualAxis(          // 0510
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );

    std::unique_ptr<UVPattern> pfCounterRibbons(    // 0515
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );

    std::unique_ptr<UVPattern> pfQuadDirectional(   // 0515-3
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );

    std::unique_ptr<UVPattern> pfPosterized(        // 0531
        uint8_t posterizeLevels = 5,
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );

    std::unique_ptr<UVPattern> pfCross(             // 0614-2
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );

    std::unique_ptr<UVPattern> pfLattice(           // standing-wave lattice
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );

    std::unique_ptr<UVPattern> pfMoire(             // moire beat field
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );

    std::unique_ptr<UVPattern> pfChladni(           // Chladni plate figure
        uint8_t modeCount = 4,
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );

    // --- PfRadialField ---
    std::unique_ptr<UVPattern> pfPetals(            // 0512
        uint8_t petalCount = 6,
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal fold = constant(500),
        S0x16Signal thickness = constant(500)
    );

    std::unique_ptr<UVPattern> pfRipple(            // 0524-2
        uint8_t waveCount = 6,
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );

    std::unique_ptr<UVPattern> pfChirp(             // radial sonar chirp
        uint8_t waveCount = 6,
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal fold = constant(500),
        S0x16Signal thickness = constant(500)
    );

    std::unique_ptr<UVPattern> pfSpiralArms(        // galaxy pinwheel arms
        uint8_t armCount = 5,
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal fold = constant(500),
        S0x16Signal thickness = constant(500)
    );

    // --- PfSourceField ---
    std::unique_ptr<UVPattern> pfRippleTank(        // N-emitter ripple tank
        uint8_t sourceCount = 3,
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );

    // --- PfContourField ---
    std::unique_ptr<UVPattern> pfOrganic(           // 0519-1
        uint8_t contourLevels = 10,
        bool hardEdges = false,
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal tension = constant(500)
    );

    std::unique_ptr<UVPattern> pfTopographic(       // 0529
        uint8_t contourLevels = 8,
        bool hardEdges = false,
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal tension = constant(500)
    );

    // --- PfPlasmaWarp ---
    std::unique_ptr<UVPattern> pfPlasma(            // A Big Hit
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );

    std::unique_ptr<UVPattern> pfTendrils(          // 0520
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );

    std::unique_ptr<UVPattern> pfLiquidGate(        // 0530
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );

    // --- PfCellularField ---
    std::unique_ptr<UVPattern> pfConcentricGrid(    // Origin
        uint8_t cellCount = 6,
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );

    std::unique_ptr<UVPattern> pfRowSegments(       // 0511 (approx)
        uint8_t cellCount = 6,
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );

    std::unique_ptr<UVPattern> pfShapes(            // 0514
        uint8_t cellCount = 6,
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );

    std::unique_ptr<UVPattern> pfDots(              // 0528
        uint8_t cellCount = 6,
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );

    std::unique_ptr<UVPattern> pfWaveMatrix(        // 0601
        uint8_t cellCount = 6,
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );

    std::unique_ptr<UVPattern> pfRadialPulse(       // 0624 (approx)
        uint8_t cellCount = 6,
        S0x16Signal phaseSpeed = constant(500),
        S0x16Signal warp = constant(500),
        S0x16Signal thickness = constant(500)
    );
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PATTERNFLOW_H
