# Transforms

A **transform** is a small, single-responsibility warp applied to a pattern in sequence. Geometric
transforms reshape the coordinate space; the palette transform reshapes color. They can be stacked,
reordered, and reused. See [Core Concepts](Core-Concepts.md) for where they sit in the pipeline.

Most transform parameters accept a [signal](Signals.md), so they can animate over time.

## Geometric transforms

- **Rotation** — rotates the coordinate space by an angle signal.
- **Zoom** — isotropic scale by a magnitude signal.
- **Translation** — 2-D Cartesian offset by a signal.
- **Vortex** — swirl whose twist grows with radius. The composer offers a **Fibonacci twist** quick-set
  (φ CW / φ CCW) that sets the golden-angle (≈137.5°) rim twist; it emits an ordinary constant signal, so
  the `.psc` wire format is unchanged.
- **Kaleidoscope** — N-fold mirror folding.
- **Radial Kaleidoscope** — radial mirror variant with configurable divisions.
- **Tiling** — repeat/tile the space with an optional mirror and cell size.
- **Flow Field** — advect coordinates along a vector field.

## Palette transform

- **Palette** — maps intensity to color through the active palette, with hue remap, intensity
  clipping/feathering, and a single-color tint mode. Raster (pixel-grid) patterns accept only
  palette-domain transforms.

Sources live under `src/renderer/pipeline/transforms/`.
