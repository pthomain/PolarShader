# Transforms

This folder contains the pipeline transforms that modify pattern sampling coordinates in either
Cartesian (Q24.8) or Polar (Q0.16 turns) space, plus palette transforms that influence final
palette lookup. Transforms are evaluated per-frame to update internal state, then applied per-sample
by wrapping a layer function (or by updating shared context).

## Domains and units

- **UV domain (Unified):** Uses `UV` coordinates (two `FracQ16_16` values) representing a normalized [0, 1] spatial domain. This is the new standard for all spatial transformations.
- **Polar domain (Legacy):** Uses `FracQ0_16` turns for angle and `FracQ0_16` for radius.
- **Cartesian domain (Legacy):** Uses `CartQ24_8` fixed-point coordinates (Q0.16 lattice units with extra precision).
- Domain changes happen through pipeline steps (`toPolar`, `toCartesian`) or by using unified `UVTransform` steps.

## Signals and ranges

All transform inputs are time-indexed signals:

- `SFracQ0_16Signal` is a function `TimeMillis -> SFracQ0_16` (usually 0..1 in Q0.16).
- `MappedValue<T>` represents a value that has already been mapped by a range.
- Ranges own all signal mapping via `map` and `mapSignal`.
- Transforms store only `MappedSignal` values, not raw `SFracQ0_16Signal` inputs.
- `ZoomMaths` owns normalization of zoom scale back to 0..1 for `PipelineContext::zoomNormalized`.
- Avoid pre-mapping or scaling signals manually; pass normalized signals and let the transform range
  do the conversion.

Ranges used by transforms:

- `PolarRange` maps `SFracQ0_16` to angular turns with proper wrapping.
- `ScalarRange` maps a 0..1 signal into an integer range `[min, max]` (used for speed/strength).
- `SFracRange` maps a 0..1 signal into a signed Q0.16 range.
- `ZoomRange` maps a 0..1 signal into the zoom scale range.
- `PaletteRange` maps a 0..1 signal into a palette index range `[0, 255]`.
- `CartRange` maps a 0..1 signal into a `CartQ24_8` range.

## Pipeline usage

Transforms are intended to be added via `PolarPipelineBuilder`, which also sets a shared
`PipelineContext` on all transforms and patterns. `advanceFrame(time)` must be called each frame
to update transform state before sampling layers.

Example (simplified):

```cpp
auto pipeline = PolarPipelineBuilder(std::move(pattern), palette, "demo")
    .addCartesianTransform(ZoomTransform(noise(cPerMil(120))))
    .addCartesianTransform(DomainWarpTransform(
        noise(cPerMil(120)),
        cPerMil(250),
        full(),
        full(),
        CartRange(CartQ24_8(1 << CARTESIAN_FRAC_BITS), CartQ24_8(1 << CARTESIAN_FRAC_BITS)),
        CartRange(CartQ24_8(4 << CARTESIAN_FRAC_BITS), CartQ24_8(4 << CARTESIAN_FRAC_BITS))
    ))
    .addPolarTransform(RotationTransform(noise(cPerMil(80))))
    .build();

pipeline.advanceFrame(timeInMillis);
auto layer = pipeline.build();
```

## Transform details

### RotationTransform (polar)

- Input: `angle` signal (turns, Q0.16).
- Uses `PolarRange` to map the resolved signal into an angular offset (0..1 turns).
- `advanceFrame` caches the mapped angle; `operator()` samples the wrapped layer at that offset so
  the transform follows whatever value was resolved from the signal (absolute values pass through,
  relative deltas are accumulated before mapping).

### VortexTransform (polar)

- Input: `strength` signal (turns, Q0.16).
- Uses `SFracRange` to map the signal into a signed angular strength.
- `operator()` adds `radius * strength` to the incoming angle, producing a radial twist.

### KaleidoscopeTransform (polar)

- Inputs: `nbFacets` (number of radial wedges), `isMirrored` (mirror every other wedge).
- Folds the angle into `nbFacets` equal sectors.
- If `isMirrored` is true, odd sectors are reflected for a symmetric kaleidoscope effect.

### TranslationTransform (cartesian)

- Inputs: `direction` (turns, Q0.16) and `speed` (0..1, Q0.16).
- Uses `PolarRange` for direction and `ScalarRange` for speed, then integrates with `CartesianMotionAccumulator`.
- Applies smoothing based on `PipelineContext::zoomNormalized` (higher zoom -> less smoothing).
- Result is an offset added to every `(x, y)` sample.

### ZoomTransform (cartesian)

- Input: `scale` signal (0..1, Q0.16).
- Uses `ZoomRange` to map into `[MIN_SCALE, MAX_SCALE]` and smooths changes based on current frequency.
- Updates `PipelineContext::zoomScale` and `zoomNormalized` each frame.

### DomainWarpTransform (cartesian)

- Inputs:
  - `phaseSpeed`: turns-per-second for the time axis (mapped via `SFracRange`). Signed; negative values reverse motion.
  - `amplitude`: 0..1 signal mapped to `[0, maxOffset]` (Q24.8). Controls warp displacement strength.
  - `warpScale`: 0..1 signal mapped via `CartRange`, applied to input coords before sampling warp noise (Q24.8).
    - Smaller than 1.0 => lower-frequency, broad bends.
    - Larger than 1.0 => higher-frequency, tighter warps.
    - Does not change displacement magnitude; only the spatial frequency of the warp field.
  - `maxOffset`: 0..1 signal mapped via `CartRange`, maximum warp displacement (Q24.8).
    - This is the dominant control for “how much” warp you see.
    - Displacement is roughly `amplitude * maxOffset * noise`, where noise is centered around 0.
    - If `maxOffset` is too small relative to your coordinate scale (Q0.16 ≈ 0..65535),
      the effect can appear invisible.
  - For `Directional` warp:
    - `flowDirection`: angular direction of the bias vector (mapped with `PolarRange`).
    - `flowStrength`: 0..1 signal mapped to `[0, maxOffset]` (Q24.8) for directional drift strength.

Example values (Q24.8 constants shown as `CartQ24_8(n << CARTESIAN_FRAC_BITS)`):

- WarpScale (frequency control):
  - Broad/slow field: `CartQ24_8(1 << CARTESIAN_FRAC_BITS)` (1.0)
  - Medium detail: `CartQ24_8(2 << CARTESIAN_FRAC_BITS)` (2.0)
  - Tight/busy: `CartQ24_8(4 << CARTESIAN_FRAC_BITS)` (4.0)

- MaxOffset (displacement strength):
  - Subtle: `CartQ24_8(256 << CARTESIAN_FRAC_BITS)`
  - Medium: `CartQ24_8(1024 << CARTESIAN_FRAC_BITS)`
  - Strong: `CartQ24_8(4096 << CARTESIAN_FRAC_BITS)`

- Typical presets:
  - `phaseSpeed`: `noise(cPerMil(80))` for gentle drift, `noise(cPerMil(200))` for faster motion.
  - `amplitude`: `cPerMil(200)` (20%) subtle, `cPerMil(500)` (50%) obvious, `full()` (100%) max.
- Warp types:
  - `Basic`, `FBM`, `Nested`, `Curl`, `Polar`, `Directional`.
- `advanceFrame` updates phase (time offset), amplitude, and directional flow offset.
- `operator()` warps coordinates and samples the wrapped layer.

Presets are provided in `DomainWarpPresets.*` for common parameter sets.

### PaletteTransform (palette)

- Input: `offset` signal (0..1, Q0.16).
- Uses `PaletteRange` to map the signal into a palette index offset.
- `advanceFrame` writes the offset into `PipelineContext::paletteOffset` for use during palette lookup.

## Context expectations

Transforms rely on a shared, non-null `PipelineContext` supplied by the pipeline. If you bypass
the builder, call `setContext` on transforms and patterns yourself, and ensure a single shared
instance is used everywhere.
