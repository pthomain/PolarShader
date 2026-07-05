# PatternFlow — Attribution & Licensing

The `patternflow/` module contains LED field effects **adapted** from the
open-source project **PatternFlow** by **Seunghun LEE**.

- Upstream repository: `engmung/Patternflow`
- Upstream target: ESP32-S3 driving a 128×64 RGB matrix; each effect is a
  floating-point HSV `draw()` loop.
- Upstream pattern licence: **CC-BY-SA-4.0** (per-file SPDX headers on the
  `firmware/patternflow/presets/preset_*.h` sources; the project README lists
  "Patterns — CC-BY-SA 4.0").
- Original author of the adapted effects: **Seunghun LEE**
  ("Lineage: AI generated and curated").

## What was adapted

Only the *algorithms / field ideas* were adapted. The original float HSV pixel
loops were **re-derived into PolarShader's fixed-point, palette-driven,
UV-space pipeline** — source lines were not copied. The upstream source colour
(HSV) is dropped; PatternFlow patterns emit a scalar intensity that the
downstream PolarShader palette + transform chain colours.

## Licensing of this module

Creative Commons publishes a one-way compatibility that permits relicensing
CC-BY-SA-4.0 material **specifically into GPLv3**. Because that compatibility
names GPLv3 (not an open-ended "or later"):

- **Adapted files** (files whose field algorithm derives from a PatternFlow
  effect) carry SPDX `GPL-3.0-only` **and** the attribution block below.
- **Original files** written from scratch for this module (e.g.
  `PfFieldMaths.h`, the tests, and the snapshot harness) carry SPDX
  `GPL-3.0-or-later`, matching the rest of PolarShader.

Every adapted `Pf…` source additionally carries a short NOTICE block naming the
upstream repository, the author, the CC-BY-SA-4.0 licence, and the source
effect date-code(s) it derives from.

## Effect date-code map

Effects are named in PolarShader by their **Variant** characteristic; the
source date-code is preserved as a code comment and as a `pf<datecode>Preset`
forwarding alias.

### All 19 effects

| Source date-code | Variant               | Primitive           |
|------------------|-----------------------|---------------------|
| Origin           | ConcentricGrid        | PfCellularField     |
| 0510             | DualAxis              | PfInterferenceField |
| 0511             | RowSegments (approx)  | PfCellularField     |
| 0512             | Petals                | PfRadialField       |
| 0514             | Shapes                | PfCellularField     |
| 0515             | CounterRibbons        | PfInterferenceField |
| 0515-3           | QuadDirectional       | PfInterferenceField |
| 0519-1           | Organic               | PfContourField      |
| 0520             | Tendrils              | PfPlasmaWarp        |
| 0524-2           | Ripple                | PfRadialField       |
| 0526             | KaleidoVortex (recipe)| PfRadialField (Ripple) + Vortex/RadialKaleidoscope |
| 0528             | Dots                  | PfCellularField     |
| 0529             | Topographic           | PfContourField      |
| 0530             | LiquidGate            | PfPlasmaWarp        |
| 0531             | Posterized            | PfInterferenceField |
| 0601             | WaveMatrix            | PfCellularField     |
| A Big Hit        | Plasma                | PfPlasmaWarp        |
| 0614-2           | Cross                 | PfInterferenceField + Tiling |
| 0624             | RadialPulse (approx)  | PfCellularField     |

`0526` (KaleidoVortex) is not a standalone pattern: it reuses the `Ripple`
field folded by a `RadialKaleidoscopeTransform` and twisted by a
`VortexTransform` (a transform recipe in `pfKaleidoVortexPreset`). `0614-2`
(Cross) folds the single breathing cross into a grid via a `TilingTransform`
in `pfCrossPreset`.

## Approximations (flagged)

- **0511 / RowSegments** and **0624 / RadialPulse** are idiomatic
  approximations of the source topology, not faithful reproductions (0624's
  nested isometric pillars re-expressed as a radial cell-height pulse field).
- **0526 / KaleidoVortex** and **0524-2 / Ripple** approximate the source's
  refraction / distance-field look with fixed-point sine-band fields plus a
  transform stack; no true per-pixel refraction sampling.
- Source colour is intentionally not reproduced (scalar-intensity + palette
  model).
