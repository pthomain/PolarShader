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

### Run

```sh
pio run -e native_pf_snapshot && .pio/build/native_pf_snapshot/program
```

Output lands in `build/pf_snapshots/` (`pattern_<name>.pgm`). View with any
image viewer that reads Netpbm, or convert:
`magick build/pf_snapshots/pattern_pfPlasma.pgm pattern_pfPlasma.png`.

### Notes

- `pfRowSegments` (source date-code `0511`) is an idiomatic **approximation** of
  the original effect.
- Intrinsically few-level fields (`pfCounterRibbons`, `pfPosterized`,
  `pfWaveMatrix`) render with a small number of distinct bands by design.
