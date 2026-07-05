//  SPDX-License-Identifier: GPL-3.0-only

/*
 * PatternFlow adaptation NOTICE
 * -----------------------------
 * Preset transform/palette recipes are original PolarShader composition over
 * the adapted PatternFlow field patterns (engmung/Patternflow, author
 * Seunghun LEE, CC-BY-SA-4.0). GPL-3.0-only via the CC-BY-SA-4.0 -> GPLv3 path.
 * See patternflow/ATTRIBUTION.md.
 *
 * All presets are deterministic (no random / seed-dependent defaults).
 */

#include "renderer/pipeline/patterns/patternflow/PatternFlowPresets.h"
#include "renderer/pipeline/transforms/RotationTransform.h"
#include "renderer/pipeline/transforms/ZoomTransform.h"
#include "renderer/pipeline/transforms/VortexTransform.h"
#include "renderer/pipeline/transforms/RadialKaleidoscopeTransform.h"
#include "renderer/pipeline/transforms/TilingTransform.h"
#include <renderer/pipeline/signals/Signals.h>
#include <utility>

namespace PolarShader {
    namespace {
        LayerBuilder makePfBuilder(
            std::unique_ptr<UVPattern> pattern,
            const CRGBPalette16 &palette,
            const char *name
        ) {
            return {std::move(pattern), palette, name};
        }
    }

    LayerBuilder pfConcentricGridPreset(const CRGBPalette16 &palette) { // Origin
        return makePfBuilder(pfConcentricGrid(), palette, "pfConcentricGrid")
                .addPaletteTransform(PaletteTransform(sine(constant(120))))
                .addTransform(ZoomTransform(constant(400)))
                .addTransform(RotationTransform(constant(60), true));
    }

    LayerBuilder pfDualAxisPreset(const CRGBPalette16 &palette) { // 0510
        return makePfBuilder(pfDualAxis(), palette, "pfDualAxis")
                .addPaletteTransform(PaletteTransform(sine(constant(150))))
                .addTransform(ZoomTransform(constant(450)));
    }

    LayerBuilder pfRowSegmentsPreset(const CRGBPalette16 &palette) { // 0511 (approx)
        return makePfBuilder(pfRowSegments(), palette, "pfRowSegments")
                .addPaletteTransform(PaletteTransform(sine(constant(100))))
                .addTransform(ZoomTransform(constant(400)));
    }

    LayerBuilder pfPetalsPreset(const CRGBPalette16 &palette) { // 0512
        return makePfBuilder(pfPetals(), palette, "pfPetals")
                .addPaletteTransform(PaletteTransform(sine(constant(140))))
                .addTransform(RotationTransform(constant(80), true));
    }

    LayerBuilder pfShapesPreset(const CRGBPalette16 &palette) { // 0514
        return makePfBuilder(pfShapes(), palette, "pfShapes")
                .addPaletteTransform(PaletteTransform(sine(constant(130))))
                .addTransform(ZoomTransform(constant(450)));
    }

    LayerBuilder pfCounterRibbonsPreset(const CRGBPalette16 &palette) { // 0515
        return makePfBuilder(pfCounterRibbons(), palette, "pfCounterRibbons")
                .addPaletteTransform(PaletteTransform(sine(constant(160))))
                .addTransform(ZoomTransform(constant(450)));
    }

    LayerBuilder pfQuadDirectionalPreset(const CRGBPalette16 &palette) { // 0515-3
        return makePfBuilder(pfQuadDirectional(), palette, "pfQuadDirectional")
                .addPaletteTransform(PaletteTransform(sine(constant(150))))
                .addTransform(ZoomTransform(constant(450)));
    }

    LayerBuilder pfOrganicPreset(const CRGBPalette16 &palette) { // 0519-1
        return makePfBuilder(pfOrganic(), palette, "pfOrganic")
                .addPaletteTransform(PaletteTransform(sine(constant(120))))
                .addTransform(ZoomTransform(constant(500)));
    }

    LayerBuilder pfTendrilsPreset(const CRGBPalette16 &palette) { // 0520
        return makePfBuilder(pfTendrils(), palette, "pfTendrils")
                .addPaletteTransform(PaletteTransform(sine(constant(140))))
                .addTransform(ZoomTransform(constant(500)))
                .addTransform(RotationTransform(constant(50), true));
    }

    LayerBuilder pfRipplePreset(const CRGBPalette16 &palette) { // 0524-2
        return makePfBuilder(pfRipple(), palette, "pfRipple")
                .addPaletteTransform(PaletteTransform(sine(constant(150))))
                .addTransform(ZoomTransform(constant(450)))
                .addTransform(RotationTransform(constant(40), true));
    }

    // 0526: reuses the Ripple field, folded by a radial kaleidoscope and
    // twisted by a vortex (transform recipe, mirroring vortexCapturePreset).
    LayerBuilder pfKaleidoVortexPreset(const CRGBPalette16 &palette) { // 0526
        return makePfBuilder(pfRipple(), palette, "pfKaleidoVortex")
                .addPaletteTransform(PaletteTransform(sine(constant(160))))
                .addTransform(ZoomTransform(constant(450)))
                .addTransform(VortexTransform(constant(500)))
                .addTransform(RadialKaleidoscopeTransform(6, true));
    }

    LayerBuilder pfDotsPreset(const CRGBPalette16 &palette) { // 0528
        return makePfBuilder(pfDots(), palette, "pfDots")
                .addPaletteTransform(PaletteTransform(sine(constant(150))))
                .addTransform(ZoomTransform(constant(400)));
    }

    LayerBuilder pfTopographicPreset(const CRGBPalette16 &palette) { // 0529
        return makePfBuilder(pfTopographic(), palette, "pfTopographic")
                .addPaletteTransform(PaletteTransform(sine(constant(120))))
                .addTransform(ZoomTransform(constant(500)));
    }

    LayerBuilder pfLiquidGatePreset(const CRGBPalette16 &palette) { // 0530
        return makePfBuilder(pfLiquidGate(), palette, "pfLiquidGate")
                .addPaletteTransform(PaletteTransform(sine(constant(170))))
                .addTransform(ZoomTransform(constant(500)))
                .addTransform(RotationTransform(constant(45), true));
    }

    LayerBuilder pfPosterizedPreset(const CRGBPalette16 &palette) { // 0531
        return makePfBuilder(pfPosterized(), palette, "pfPosterized")
                .addPaletteTransform(PaletteTransform(sine(constant(160))))
                .addTransform(ZoomTransform(constant(450)));
    }

    LayerBuilder pfWaveMatrixPreset(const CRGBPalette16 &palette) { // 0601
        return makePfBuilder(pfWaveMatrix(), palette, "pfWaveMatrix")
                .addPaletteTransform(PaletteTransform(sine(constant(150))))
                .addTransform(ZoomTransform(constant(400)));
    }

    LayerBuilder pfPlasmaPreset(const CRGBPalette16 &palette) { // A Big Hit
        return makePfBuilder(pfPlasma(), palette, "pfPlasma")
                .addPaletteTransform(PaletteTransform(sine(constant(140))))
                .addTransform(ZoomTransform(constant(500)))
                .addTransform(RotationTransform(constant(40), true));
    }

    // 0614-2: the single breathing cross is folded into a grid of crosses by a
    // tiling transform (the fold is a transform, not a pattern param). Tiles use
    // the full cell size so each tile spans the pattern's [0,1] domain (the cross
    // is centred at 0.5), and a zoom-out scales the coordinate space so several
    // whole tiles are visible. Transforms execute outermost-first, so the added
    // order is pattern <- Tiling <- Zoom <- Rotation.
    LayerBuilder pfCrossPreset(const CRGBPalette16 &palette) { // 0614-2
        return makePfBuilder(pfCross(), palette, "pfCross")
                .addPaletteTransform(PaletteTransform(sine(constant(150))))
                .addTransform(TilingTransform(constant(1000), false))
                .addTransform(ZoomTransform(constant(355)))
                .addTransform(RotationTransform(constant(30), true));
    }

    LayerBuilder pfRadialPulsePreset(const CRGBPalette16 &palette) { // 0624 (approx)
        return makePfBuilder(pfRadialPulse(), palette, "pfRadialPulse")
                .addPaletteTransform(PaletteTransform(sine(constant(160))))
                .addTransform(ZoomTransform(constant(450)));
    }
}
