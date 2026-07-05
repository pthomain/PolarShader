//  SPDX-License-Identifier: GPL-3.0-only

/*
 * PatternFlow adaptation NOTICE
 * -----------------------------
 * Presets (pattern + transforms + palette) for the PatternFlow field family
 * adapted from engmung/Patternflow (author Seunghun LEE, CC-BY-SA-4.0). Only
 * the field algorithms are adapted; the transform/palette recipes here are
 * original PolarShader composition. GPL-3.0-only via the CC-BY-SA-4.0 -> GPLv3
 * one-way compatibility path. See patternflow/ATTRIBUTION.md.
 */

#ifndef POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PATTERNFLOWPRESETS_H
#define POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PATTERNFLOWPRESETS_H

#include "renderer/layer/LayerBuilder.h"
#include "renderer/pipeline/patterns/patternflow/PatternFlow.h"

namespace PolarShader {
    /*
     * Canonical presets, named by field Variant. Each builds its pf* pattern and
     * attaches the effect's transform + palette stack, returning a LayerBuilder
     * (mirroring presets/Presets.h). Every preset is deterministic: no random or
     * seed-dependent defaults, so two constructions produce identical frames.
     */
    LayerBuilder pfConcentricGridPreset(const CRGBPalette16 &palette);  // Origin
    LayerBuilder pfDualAxisPreset(const CRGBPalette16 &palette);        // 0510
    LayerBuilder pfRowSegmentsPreset(const CRGBPalette16 &palette);     // 0511 (approx)
    LayerBuilder pfPetalsPreset(const CRGBPalette16 &palette);          // 0512
    LayerBuilder pfShapesPreset(const CRGBPalette16 &palette);          // 0514
    LayerBuilder pfCounterRibbonsPreset(const CRGBPalette16 &palette);  // 0515
    LayerBuilder pfQuadDirectionalPreset(const CRGBPalette16 &palette); // 0515-3
    LayerBuilder pfOrganicPreset(const CRGBPalette16 &palette);         // 0519-1
    LayerBuilder pfTendrilsPreset(const CRGBPalette16 &palette);        // 0520
    LayerBuilder pfRipplePreset(const CRGBPalette16 &palette);          // 0524-2
    LayerBuilder pfKaleidoVortexPreset(const CRGBPalette16 &palette);   // 0526
    LayerBuilder pfDotsPreset(const CRGBPalette16 &palette);            // 0528
    LayerBuilder pfTopographicPreset(const CRGBPalette16 &palette);     // 0529
    LayerBuilder pfLiquidGatePreset(const CRGBPalette16 &palette);      // 0530
    LayerBuilder pfPosterizedPreset(const CRGBPalette16 &palette);      // 0531
    LayerBuilder pfWaveMatrixPreset(const CRGBPalette16 &palette);      // 0601
    LayerBuilder pfPlasmaPreset(const CRGBPalette16 &palette);          // A Big Hit
    LayerBuilder pfCrossPreset(const CRGBPalette16 &palette);           // 0614-2
    LayerBuilder pfRadialPulsePreset(const CRGBPalette16 &palette);     // 0624 (approx)

    /*
     * Date-code aliases: zero-cost inline forwarders to the canonical presets,
     * preserving the upstream Pf<datecode> naming. The alias symbol is
     * pf<sanitised>Preset, where every '-' in the date-code becomes '_'.
     * Non-date-code effects use a mnemonic (Origin, BigHit).
     */
    inline LayerBuilder pfOriginPreset(const CRGBPalette16 &p) { return pfConcentricGridPreset(p); }
    inline LayerBuilder pf0510Preset(const CRGBPalette16 &p) { return pfDualAxisPreset(p); }
    inline LayerBuilder pf0511Preset(const CRGBPalette16 &p) { return pfRowSegmentsPreset(p); }
    inline LayerBuilder pf0512Preset(const CRGBPalette16 &p) { return pfPetalsPreset(p); }
    inline LayerBuilder pf0514Preset(const CRGBPalette16 &p) { return pfShapesPreset(p); }
    inline LayerBuilder pf0515Preset(const CRGBPalette16 &p) { return pfCounterRibbonsPreset(p); }
    inline LayerBuilder pf0515_3Preset(const CRGBPalette16 &p) { return pfQuadDirectionalPreset(p); }
    inline LayerBuilder pf0519_1Preset(const CRGBPalette16 &p) { return pfOrganicPreset(p); }
    inline LayerBuilder pf0520Preset(const CRGBPalette16 &p) { return pfTendrilsPreset(p); }
    inline LayerBuilder pf0524_2Preset(const CRGBPalette16 &p) { return pfRipplePreset(p); }
    inline LayerBuilder pf0526Preset(const CRGBPalette16 &p) { return pfKaleidoVortexPreset(p); }
    inline LayerBuilder pf0528Preset(const CRGBPalette16 &p) { return pfDotsPreset(p); }
    inline LayerBuilder pf0529Preset(const CRGBPalette16 &p) { return pfTopographicPreset(p); }
    inline LayerBuilder pf0530Preset(const CRGBPalette16 &p) { return pfLiquidGatePreset(p); }
    inline LayerBuilder pf0531Preset(const CRGBPalette16 &p) { return pfPosterizedPreset(p); }
    inline LayerBuilder pf0601Preset(const CRGBPalette16 &p) { return pfWaveMatrixPreset(p); }
    inline LayerBuilder pfBigHitPreset(const CRGBPalette16 &p) { return pfPlasmaPreset(p); }
    inline LayerBuilder pf0614_2Preset(const CRGBPalette16 &p) { return pfCrossPreset(p); }
    inline LayerBuilder pf0624Preset(const CRGBPalette16 &p) { return pfRadialPulsePreset(p); }

    /*
     * Public mapping table: date-code <-> variant name <-> canonical preset.
     * Stores a preset function pointer so any key builds the effect via
     * PF_EFFECTS[i].preset(palette). inline constexpr (C++17) gives a single
     * ODR-safe definition in this header.
     */
    using PfPresetFn = LayerBuilder (*)(const CRGBPalette16 &);

    struct PfEffect {
        const char *dateCode;
        const char *variant;
        PfPresetFn preset;
    };

    inline constexpr PfEffect PF_EFFECTS[19] = {
        {"Origin",    "ConcentricGrid",  &pfConcentricGridPreset},
        {"0510",      "DualAxis",        &pfDualAxisPreset},
        {"0511",      "RowSegments",     &pfRowSegmentsPreset},
        {"0512",      "Petals",          &pfPetalsPreset},
        {"0514",      "Shapes",          &pfShapesPreset},
        {"0515",      "CounterRibbons",  &pfCounterRibbonsPreset},
        {"0515-3",    "QuadDirectional", &pfQuadDirectionalPreset},
        {"0519-1",    "Organic",         &pfOrganicPreset},
        {"0520",      "Tendrils",        &pfTendrilsPreset},
        {"0524-2",    "Ripple",          &pfRipplePreset},
        {"0526",      "KaleidoVortex",   &pfKaleidoVortexPreset},
        {"0528",      "Dots",            &pfDotsPreset},
        {"0529",      "Topographic",     &pfTopographicPreset},
        {"0530",      "LiquidGate",      &pfLiquidGatePreset},
        {"0531",      "Posterized",      &pfPosterizedPreset},
        {"0601",      "WaveMatrix",      &pfWaveMatrixPreset},
        {"A Big Hit", "Plasma",          &pfPlasmaPreset},
        {"0614-2",    "Cross",           &pfCrossPreset},
        {"0624",      "RadialPulse",     &pfRadialPulsePreset},
    };
}

#endif // POLAR_SHADER_PIPELINE_PATTERNS_PATTERNFLOW_PATTERNFLOWPRESETS_H
