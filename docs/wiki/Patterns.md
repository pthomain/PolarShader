# Patterns

A **pattern** is the base procedural sampler of a layer — it produces a value (intensity, hue+brightness,
or RGB) for every point before any transforms are applied. See [Core Concepts](Core-Concepts.md) for how
patterns fit into the pipeline.

Names below use the composer's labels. This is a catalog, not an API reference; see the sources under
`src/renderer/pipeline/patterns/` for parameters.

## Noise & fields

- **Noise** — Perlin/value noise with Basic, FBM, Turbulence, and Ridged modes.
- **Worley / Voronoi** — cellular (Worley) noise for organic cell structures.
- **Reaction-Diffusion** — Gray-Scott reaction-diffusion (continuous form).

## PatternFlow fields

**PatternFlow** composes field primitives (radial, cellular, contour, interference, source, plasma warp)
into a large family of flow-style effects. It ships with many presets, e.g. *Dual Axis*, *Counter
Ribbons*, *Quad Directional*, *Petals*, *Organic*, *Tendrils*, *Liquid Gate*, *Topographic*, *Plasma*,
*Dots*, *Wave Matrix*, *Radial Pulse*, and wave/interference presets *Lattice*, *Moiré*, *Chladni*,
*Chirp (Sonar)*, *Spiral Arms*, and *Ripple Tank*.

## Pixel-grid (raster) patterns

These run on a discrete cell grid and accept only palette-domain transforms:

- **Conway** — Conway's Game of Life.
- **Brian's Brain** — three-state Life variant.
- **Cyclic CA** — cyclic cellular automaton.
- **Elementary CA** — 1-D elementary cellular automata.
- **Life Variants** — configurable Life-like rules.
- **Forest Fire** — fire-spread model with growth and lightning.
- **Langton's Ant** — turmite exploration.
- **WireWorld** — WireWorld logic simulation.
- **Matrix Rain** — falling digital-rain effect.
- **Ripple** — expanding ripple waves.
- **Reaction-Diffusion (raster)** — discrete-grid Gray-Scott.
- **XOR — Munching Squares** — classic XOR bit pattern.

## ShaderToy ports

RGB-native ports of ShaderToy shaders: **Palette Glow**, **Rocaille**, **Protean Clouds**, **Octgrams**,
**Rotating Squares**, **Starry Planes**, **Trig Field**, and **Star Field Travel**.

## Geometric & motion

- **Spiral**, **Annuli** (concentric rings), **Tiling** (square/triangle/hex).
- **Flow Field**, **Transport** (advection/drift), **Flurry** (particle bursts).
