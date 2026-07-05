<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

# Native tools

Standalone desktop utilities. Each tool has its own `main()` and is compiled by
exactly one dedicated PlatformIO env; every other env excludes it from
`build_src_filter` so no second `main()` is ever linked. They live under `src/`
because PlatformIO's `build_src_filter` only selects files inside the project
`src` tree.

## `pf_snapshot.cpp` — PatternFlow snapshot dumper

Writes visual proofs of the ported PatternFlow effects into
`build/pf_snapshots/`:

- **PGM per bare `pf*` pattern** — samples `layer()(UV)` over a Cartesian UV
  grid (grayscale intensity field, no transforms/palette).
- **PPM per `pf*Preset` builder** — samples the built `Layer::compile()`
  `ColourMap` over a polar display disc `(angle, radius) -> CRGB`. This is the
  only view that exercises the full transform + palette stack, so it is the
  proof that transform-dependent presets actually apply their
  rotation/zoom/palette-offset (and, for effects that use them,
  kaleidoscope/vortex/tiling) stack.

The PPM uses a full-saturation hue wheel palette (deterministic, distinct across
the wheel) so the image shows field structure rather than a palette artefact.

### Run

```sh
pio run -e native_pf_snapshot && .pio/build/native_pf_snapshot/program
```

Output lands in `build/pf_snapshots/` (`pattern_<name>.pgm`,
`preset_<Variant>.ppm`). View with any image viewer that reads Netpbm, or
convert: `magick build/pf_snapshots/preset_Plasma.ppm preset_Plasma.png`.

### Notes

- `pfRowSegments` (source date-code `0511`) is an idiomatic **approximation** of
  the original effect.
- Intrinsically few-level fields (`pfCounterRibbons`, `pfPosterized`,
  `pfWaveMatrix`) render with a small number of distinct bands by design.
